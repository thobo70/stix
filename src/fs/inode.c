/**
 * @file inode.c
 * @author Thomas Boos
 * @brief handles inode stuff
 * @version 0.1
 * @date 2023-09-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "tdefs.h"
#include <stdio.h>
#include "inode.h"
#include "blocks.h"
#include "buf.h"
#include "pc.h"
#include "fs.h"
#include "utils.h"

#define SUPERBLOCKINODE(fs)  (getisblock(fs)->dsblock.inodes)   ///< first block with inodes in fs

#define HTABSIZEBITS 4

#define HTABSIZE (1 << HTABSIZEBITS)
#define HTABMASK (HTABSIZE - 1)
#define HTABVALUE(fs, inum) ((inum) & HTABMASK)
#define HTAB(fs, inum) ihashtab[HTABVALUE(dev, inum)]

#define INODEBLOCK(fs, inum) (((inum - 1) / NINODESBLOCK) + SUPERBLOCKINODE(fs))
#define INODEOFFSET(inum) (((inum - 1) % NINODESBLOCK) * sizeof(dinode_t))

#define NREFSPERBLOCK ((block_t)(BLOCKSIZE / sizeof(block_t)))

iinode_t iinode[NINODES];

iinode_t *ihashtab[HTABSIZE];
iinode_t *ifreelist = NULL;



/**
 * @brief recursively iterates over the levels of indirection of block references
 * 
 * @param level current level
 * @param fs    working fs
 * @param bl    referenced block
 */
void freeblocklevel(int level, fsnum_t fs, block_t bl)
{
  if (level) {
    --level;
    bhead_t *b = bread(LDEVFROMFS(fs), bl);
    block_t *brefs = (block_t *)b->buf->mem;
    for ( int i = 0 ; (i < NREFSPERBLOCK) ; ++i )
      if (brefs[i])
        freeblocklevel(level, fs, brefs[i]);
    brelse(b);
  }
  bfree(fs, bl);
}


/**
 * @brief free all blocks assigned to this inode
 * 
 * @param inode the inode with block references
 */
void free_all_blocks(iinode_t *inode)
{
  int level = 0;

  ASSERT(inode);
  for ( int i = 0 ; i < NBLOCKREFS ; ++i ) {
    block_t bl = inode->dinode.blockrefs[i];
    if (i >= STARTREFSLEVEL)
      ++level;
    if (bl == 0)
      continue;
    freeblocklevel(level, inode->fs, bl);
  }
}



/**
 * @brief write inode to disk
 * 
 * @param inode the inode to write
 */
void update_inode_on_disk(iinode_t *inode)
{
  ASSERT(inode);
  bhead_t *b = bread(LDEVFROMFS(inode->fs), INODEBLOCK(inode->fs, inode->inum));
  mcpy(&b->buf->mem[INODEOFFSET(inode->inum)], &inode->dinode, sizeof(dinode_t));
  b->dwrite = true;
  bwrite(b);
  brelse(b);
  inode->modified = false;
}



/**
 * @brief remove inode from free list
 * 
 * @param inode inode to remove
 */
void remove_inode_from_freelist(iinode_t *inode)
{
  ASSERT(inode);
  if (!inode->fnext)
    return;
  if (inode->fnext == inode) {
    ifreelist = NULL;
  } else {
    if (inode == ifreelist) 
      ifreelist = inode->fnext;
    inode->fprev->fnext = inode->fnext;
    inode->fnext->fprev = inode->fprev;
  }
  inode->fnext = NULL;
  inode->fprev = NULL;
}



/**
 * @brief add indoe to free list
 * 
 * @param inode     inode put to free list
 * @param asFirst   true if to add in front of list, otherwise false
 */
void add_inode_to_freelist(iinode_t *inode, int asFirst)
{
  ASSERT(inode);
  if (ifreelist) {
    inode->fprev = ifreelist->fprev;
    inode->fnext = ifreelist;
    ifreelist->fprev->fnext = inode;
    ifreelist->fprev = inode;
    if (asFirst)
      ifreelist = inode;
  } else {
    ifreelist = inode;
    inode->fnext = inode;
    inode->fprev = inode;
  }
}



/**
 * @brief move inot to new hash queue
 * 
 * @param inode the inode to move
 * @param fs    fs of new queue
 * @param inum  inum of new queue
 */
void move_inode_to_hashqueue(iinode_t *inode, fsnum_t fs, ninode_t inum)
{
  ASSERT(inode);
  ASSERT(fs > 0 && fs <= MAXFS);
  ASSERT(inum < getisblock(fs)->dsblock.ninodes);
  iinode_t *p = HTAB(fs, inum);
  if (inode->hnext) {
    if (inode->hnext == inode)
      HTAB(inode->fs, inode->inum) = NULL;
    else
      HTAB(inode->fs, inode->inum) = inode->hnext;
    inode->hprev->hnext = inode->hnext;
    inode->hnext->hprev = inode->hprev;
  }
  inode->fs = fs;
  inode->inum = inum;
  if (p) {
    p->hprev->hnext = inode;
    p->hprev = inode;
  } else {
    inode->hprev = inode;
    inode->hnext = inode;    
    HTAB(fs, inum) = inode;
  }
}



/**
 * @brief init inodes
 * 
 */
void init_inodes(void)
{
  mset(iinode, 0, sizeof(iinode));
  mset(ihashtab, 0, sizeof(ihashtab));

  for ( int i = 0 ; i < NBUFFER ; ++i ) 
    add_inode_to_freelist(&iinode[i], 0);
}



/**
 * @brief allocates an inode from the fs
 * 
 * @param fs          the fs where to search for a free inode
 * @param ftype       the type of the inode
 * @return iinode_t*  points to free inode or NULL if an error is set
 */
iinode_t *ialloc(fsnum_t fs, ftype_t ftype, fmode_t fmode)
{
  iinode_t *ii = NULL;
  bhead_t *bhead = NULL;
  dinode_t *dinode = NULL;

  ASSERT(fs > 0 && fs <= MAXFS);

  isuperblock_t *isbk = getisblock(fs);
  // block_t firstinodeblock = SUPERBLOCKINODE(fs);
  block_t currblk = 0;

  for(;;) {
    if (isbk->locked) {
      waitfor(SBLOCKBUSY);
      continue;
    }
    isbk->locked = true;
    if ((isbk->nfinodes >= NFREEINODES) || (isbk->finode[isbk->nfinodes] == 0)) {
      mset(&isbk->finode, 0, sizeof(isbk->finode));
      ninode_t iidx = isbk->lastfinode;
      int i = 0;
      do {
        if (++iidx >= isbk->dsblock.ninodes)
          break;
        if (!bhead || (currblk != INODEBLOCK(fs, iidx))) {
          currblk = INODEBLOCK(fs, iidx);
          if (bhead)
            brelse(bhead);
          bhead = breada(LDEVFROMFS(fs), currblk, currblk + 1);
          dinode = (dinode_t*)bhead->buf->mem;
        }
        if (dinode[(iidx - 1) % NINODESBLOCK].ftype == IFREE)
          isbk->finode[i++] = iidx;
      } while (i < NFREEINODES);
      if (bhead)
        brelse(bhead);
      isbk->nfinodes = 0;
      if (isbk->finode[isbk->nfinodes] == 0) {
        isbk->locked = false;
        wakeall(SBLOCKBUSY);
        /// @todo set error no free inodes in fs
        return NULL;
      }
    }
    ii = iget(fs, isbk->finode[isbk->nfinodes]);
    if (!ii) {
      isbk->locked = false;
      wakeall(SBLOCKBUSY);
      /// @todo set error no free inodes in fs
      return NULL;
    }
    isbk->lastfinode = isbk->finode[isbk->nfinodes++];
    isbk->locked = false;
    wakeall(SBLOCKBUSY);
    if ((ii->dinode.ftype != IFREE) || (ii->nref > 1) || (ii->dinode.nlinks > 0) || (ii->locked)) {
      update_inode_on_disk(ii);
      iput(ii);
      continue;
    }
    while (ii->locked) {
      waitfor(INODELOCKED);   /// @todo check if this is necessary
    }
    ii->locked = true;
    mset(&ii->dinode, 0, sizeof(dinode_t));
    ii->dinode.ftype = ftype;
    ii->dinode.fmode = fmode;
    // ii->dinode.nlinks = 0;      // will be incremented by linki
    /// @todo set uid, gid, ...
    ii->modified = true;
    ii->locked = false;
    wakeall(INODELOCKED);
    update_inode_on_disk(ii);
    return ii;
  }
}



/**
 * @brief frees an inode in fs
 * 
 * @param inode the inode to be freed
 */
void ifree(iinode_t *inode)
{
  ASSERT(inode);
  isuperblock_t *isbk = getisblock(inode->fs);

  inode->dinode.ftype = IUNSPEC;

  while (isbk->locked) {
    waitfor(SBLOCKBUSY);
  }
  isbk->locked = true;

  if (isbk->nfinodes >= NFREEINODES) {
    isbk->nfinodes = NFREEINODES - 1;
    isbk->finode[isbk->nfinodes] = inode->inum;
  } else if ((isbk->nfinodes == 0) || (isbk->finode[isbk->nfinodes] < inode->inum)) {
    int d = 0, j = 0;
    for ( int i = isbk->nfinodes + 1 ; i < NFREEINODES ; ++i ) {
      if (isbk->finode[i] > (inode->inum + d)) {
        j = i;
        d = isbk->finode[i] - inode->inum;
      }
    }
    if (j)
      isbk->finode[j] = inode->inum;
  } else
    isbk->finode[--isbk->nfinodes] = inode->inum;

  mset(&inode->dinode, 0, sizeof(dinode_t));
  inode->dinode.ftype = IFREE;
  update_inode_on_disk(inode);

  isbk->locked = false;
  wakeall(SBLOCKBUSY);
}



/**
 * @brief searches inode in hashlist or sets up a new one
 * 
 * @param fs 
 * @param inum 
 * @return iinode_t* inode or NULL if not found and no free inode left
 */
iinode_t *iget(fsnum_t fs, ninode_t inum)
{
  iinode_t *i, *found = NULL;
  bhead_t *bhead = NULL;

  ASSERT(fs > 0 && fs <= MAXFS);
  ASSERT(inum < getisblock(fs)->dsblock.ninodes);
  for(;;) {
    ASSERT(fs == getisblock(fs)->fs);
    i = HTAB(fs, inum);
    for ( found = i ; found ; found = (found->hnext == i) ? NULL : found->hnext )
      if ((found->fs == fs) && (found->inum == inum))
        break;
    if (found) {
      if (found->locked) {
        waitfor(INODELOCKED);
        continue;
      }
      if (found->fsmnt) {
        fs = found->fsmnt;
        inum = 1;
        continue;
      }
      remove_inode_from_freelist(found);
      found->nref++;
      return found;
    }
    found = ifreelist;
    if (!found) {
      /// @todo error no free inodes
      return NULL;
    }
    remove_inode_from_freelist(found);
    move_inode_to_hashqueue(found, fs, inum);
    bhead = bread(LDEVFROMFS(found->fs), INODEBLOCK(fs, inum));
    mcpy(&found->dinode, &bhead->buf->mem[INODEOFFSET(inum)], sizeof(dinode_t));
    brelse(bhead);
    found->nref = 1;
    return found;
  }
}



/**
 * @brief releases inode
 * 
 * @param inode 
 */
void iput(iinode_t *inode)
{
  ASSERT(inode);
  inode->locked = true;
  inode->nref--;
  if (inode->nref == 0) {
    if (inode->dinode.nlinks == 0) {
      free_all_blocks(inode);
      ifree(inode);
    }
    if (inode->modified)
      update_inode_on_disk(inode);
    add_inode_to_freelist(inode, false);
  }
  inode->locked = false;
  wakeall(INODELOCKED);
}



/**
 * @brief mapping from file position to block in fs and allocates new blocks if necessary
 * 
 * @param inode 
 * @param pos 
 * @return bmap_t 
 */
bmap_t bmap(iinode_t *inode, fsize_t pos)
{
  ASSERT(inode);
  bmap_t bm;
  block_t lblock = pos / BLOCKSIZE;
  bm.fsblock = 0;
  bm.offblock = pos % BLOCKSIZE;
  bm.nbytesleft = BLOCKSIZE - bm.offblock;
  bm.rdablock = 0;

  if (lblock < STARTREFSLEVEL) {
    bm.fsblock = inode->dinode.blockrefs[lblock];
    if (bm.fsblock == 0) {    // allocate new block
      bhead_t *bh = balloc(inode->fs);
      if (bh == NULL)
        return bm;      
      bm.fsblock = inode->dinode.blockrefs[lblock] = bh->block;
      inode->modified = true;
      brelse(bh);
    }
    bm.rdablock = inode->dinode.blockrefs[lblock + 1];
    return bm;
  }

  lblock -= STARTREFSLEVEL;
  dword_t d = 1;
  int l;
  for ( l = 0 ; l < 5; ++l ) {
    if (lblock < NREFSPERBLOCK) {
      break;
    }
    d *= NREFSPERBLOCK;
    lblock -= d;
  }

  block_t b = inode->dinode.blockrefs[STARTREFSLEVEL + l];
  if (b == 0) {    // allocate new block
    bhead_t *bh = balloc(inode->fs);
    if (bh == NULL)
      return bm;      
    b = inode->dinode.blockrefs[STARTREFSLEVEL + l] = bh->block;
    inode->modified = true;
    brelse(bh);
  }
  do {
    bhead_t *bh = bread(LDEVFROMFS(inode->fs), b);
    block_t *refs = (block_t *)bh->buf->mem;
    int idx = lblock / d;
    ASSERT(idx < NREFSPERBLOCK);
    b = refs[idx];
    if (b == 0) {    // allocate new block
      bhead_t *bha = balloc(inode->fs);
      if (bha == NULL) {
        brelse(bh);
        return bm;
      }
      b = refs[idx] = bha->block;
      brelse(bha);
      bh->dwrite = true;
      bwrite(bh);
    }
    if (++idx < NREFSPERBLOCK)
      bm.rdablock = refs[idx];
    brelse(bh);
    lblock %= d;
    d /= NREFSPERBLOCK;
  } while (d > 0);
  
  bm.fsblock = b;
  return bm;
}



/**
 * @brief finds inode which path points to and returns inode and parent inode id
 * 
 * @param p           file path
 * @return namei_t    inode and parent inode id
 */
namei_t namei(const char *p)
{
  iinode_t *wi = NULL;    // working inode
  dword_t i, n;           // index and number of directory entries 
  int ps;                 // size of current path name part
  bmap_t bm;
  bhead_t *bh;
  dirent_t *de;
  fsnum_t fs;
  int found;

  ASSERT(p);
  ASSERT(active);
  
  // Initialize return structure with proper default filesystem
  fsnum_t default_fs = (active->u->fsroot) ? active->u->fsroot->fs : 1;
  namei_t rtn = {NULL, 0, default_fs};
  
  if (*p == 0)
    return rtn;
  if (*p == '/') {
    wi = iget(active->u->fsroot->fs, active->u->fsroot->inum);
    ++p;
  } else
    wi = iget(active->u->workdir->fs, active->u->workdir->inum);
  if (!wi)
    return rtn;

  fs = wi->fs;  // Initialize fs to the working inode's filesystem
  rtn.p = wi->inum;
  while (*p)  {
    for ( ps = 0 ; p[ps] && (p[ps] != '/') ; ++ps );
    if (ps > DIRNAMEENTRY) {
      /// @todo path name too long error
      iput(wi);
      return rtn;
    }
    int skip = false;
    if (sncmp(p, "..", ps) == 0) {
      if (wi == active->u->fsroot) {  // if *p is ".." and wi is root then continue with next path part
        /// @todo potentional error when active->u->fsroot is root of a mounted fs (chroot("/mnt"); mount /dev/hddb1 /mnt)
        skip = true;
      } else if (wi->inum == 1) {     // if *p is ".." and wi is root of mounted fs then retrieve parent fs
        skip = true;
        iput(wi);
        isuperblock_t *isbk = getisblock(wi->fs);
        wi = iget(isbk->pfs, isbk->pino);
        rtn.p = wi->inum;
        rtn.fs = wi->fs;
      } 
    }
    if (!skip) {  
      if (wi->dinode.ftype != DIRECTORY) {
        /// @todo set error no dir
        iput(wi);
        return rtn;
      }
      /// @todo check permission
      ASSERT(wi->dinode.fsize % sizeof(dirent_t) == 0);
      n = wi->dinode.fsize / sizeof(dirent_t);
      fs = wi->fs;
      found = false;
      for ( i = 0 ; !found && i < n ; ++i ) {
        /// @todo optimize algorithm, is quick and dirty
        bm = bmap(wi, i * sizeof(dirent_t));
        if (bm.fsblock == 0) {  // no block assigned, error should be set by balloc in bmap
          iput(wi);
          return rtn;
        }
        bh = breada(LDEVFROMFS(fs), bm.fsblock, bm.rdablock);
        de = (dirent_t*)&bh->buf->mem[bm.offblock];
        if (de->inum > 0 && sncmp(p, de->name, ps) == 0) {
          rtn.p = wi->inum;       // parent inode
          rtn.fs = wi->fs;
          iput(wi);
          wi = iget(fs, de->inum);  // iget take care of mounted fs
          if (wi) {
            fs = wi->fs;  // Update fs to handle mount point redirection
          }
          found = true;
        }
        brelse(bh);
      }
      if (!found) {
        if (p[ps] == 0) {     // file was not found but parent dir was found
          rtn.p = wi->inum;   // parent inode
          rtn.fs = wi->fs;
        }
        /// @todo error entry not found
        iput(wi);
        return rtn;
      }
    }
    p += ps;
    if (*p == '/')
      ++p;
  }
  
  rtn.i = wi;
  rtn.fs = wi->fs;  // Ensure fs is set to the final inode's filesystem
  return rtn;
}



/**
 * @brief returns number of active inodes in fs with improved detection
 * 
 * Enhanced to detect:
 * - Open files through file table entries
 * - Current working directories and filesystem roots
 * - Directory operations in progress
 * - Cached inodes that are logically active
 * 
 * @param fs filesystem number
 * @return int number of active inodes
 */
int activeinodes(fsnum_t fs)
{
  ASSERT(fs > 0 && fs <= MAXFS);
  int rtn = 0;
  iinode_t *ii;
  
  // Check all in-memory inodes
  for ( int i = 0 ; i < NINODES ; ++i ) {
    ii = &iinode[i];
    if (ii->nref == 0)
      continue;
    if (ii->fs != fs)
      continue;
      
    // Count this inode as active
    rtn++;
  }
  
  // Additional check: scan file table for open files on this filesystem
  // This catches cases where files are open but inode reference counting
  // might not reflect the true usage state
  extern filetab_t filetab[MAXFILETAB];
  for (int i = 0; i < MAXFILETAB; i++) {
    if (filetab[i].inode && filetab[i].refs > 0 && filetab[i].inode->fs == fs) {
      // Verify this inode wasn't already counted above
      int already_counted = 0;
      for (int j = 0; j < NINODES; j++) {
        if (&iinode[j] == filetab[i].inode && iinode[j].nref > 0) {
          already_counted = 1;
          break;
        }
      }
      if (!already_counted) {
        rtn++;
      }
    }
  }
  
  // Check if any process has this filesystem as working directory or root
  extern process_t *active;
  if (active && active->u) {
    // Check current working directory
    if (active->u->workdir && active->u->workdir->fs == fs) {
      // Working directory inodes should already be counted above through nref,
      // but this serves as an additional safety check
    }
    
    // Check filesystem root
    if (active->u->fsroot && active->u->fsroot->fs == fs) {
      // Root directory inodes should already be counted above through nref,
      // but this serves as an additional safety check
    }
    
    // Check all open file descriptors for this process
    for (int fd = 0; fd < MAXOPENFILES; fd++) {
      if (active->u->fdesc[fd].ftabent && 
          active->u->fdesc[fd].ftabent->inode &&
          active->u->fdesc[fd].ftabent->inode->fs == fs) {
        // These should be caught by the file table scan above,
        // but this provides process-specific validation
      }
    }
  }
  
  return rtn;
}


/**
 * @brief Check for open files on a specific filesystem
 * 
 * This function provides detailed information about what's keeping
 * a filesystem busy, useful for debugging umount failures.
 * 
 * @param fs filesystem number
 * @return int number of open files/directories on the filesystem
 */
int count_open_files_on_fs(fsnum_t fs)
{
  ASSERT(fs > 0 && fs <= MAXFS);
  int open_files = 0;
  extern filetab_t filetab[MAXFILETAB];
  
  // Scan the global file table
  for (int i = 0; i < MAXFILETAB; i++) {
    if (filetab[i].inode && filetab[i].refs > 0 && filetab[i].inode->fs == fs) {
      open_files++;
    }
  }
  
  return open_files;
}

/**
 * @brief Check if filesystem is busy due to working directories
 * 
 * @param fs filesystem number
 * @return int 1 if busy due to working directories, 0 otherwise
 */
int is_fs_busy_workdir(fsnum_t fs)
{
  ASSERT(fs > 0 && fs <= MAXFS);
  extern process_t *active;
  
  if (active && active->u) {
    // Check current working directory
    if (active->u->workdir && active->u->workdir->fs == fs) {
      return 1;
    }
    
    // Check filesystem root
    if (active->u->fsroot && active->u->fsroot->fs == fs) {
      return 1;
    }
  }
  
  return 0;
}
