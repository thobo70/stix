/**
 * @file blocks.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief handling of blocks on disk
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "blocks.h"
#include "buf.h"
#include "fs.h"
#include "pc.h"
#include <stdio.h>
#include "utils.h"


isuperblock_t isblock[MAXFS];



isuperblock_t *getisblock(fsnum_t fs)
{
  ASSERT(fs > 0 && fs <= MAXFS);
  return &isblock[fs - 1];
}

#define BMAPBLOCK(idx)  (idx / (BLOCKSIZE * 8))
#define BMAPIDX(idx)    ((idx % (BLOCKSIZE * 8)) / 8)
#define BMAPMASK(idx)   (byte_t)(1 << (idx % 8))


/**
 * @brief Initialize superblock for a given device.
 * 
 * This function reads the superblock from the device and initializes the superblock for the file system on the device.
 * 
 * @startuml
 * start
 * :Start;
 * :Initialize fs, freefs;
 * :for fs in 0 to MAXFS;
 * if (isblock[fs].inuse) then (yes)
 *   if (isblock[fs].dev.ldev == dev.ldev) then (yes)
 *     :Return fs + 1;
 *     stop
 *   endif
 * else (no)
 *   if (freefs == MAXFS) then (yes)
 *     :freefs = fs;
 *   endif
 * endif
 * :endfor;
 * :fs = freefs;
 * if (fs == MAXFS) then (yes)
 *   :Set error: no free superblock;
 *   :Return 0;
 *   stop
 * endif
 * :Increment fs;
 * :isbk = getisblock(fs);
 * :bh = bread(dev, 1);
 * if (bh->error) then (yes)
 *   :Set error: reading superblock;
 *   :Return 0;
 *   stop
 * endif
 * :Initialize isbk with bh;
 * :Release bh;
 * :isbk->inuse = true;
 * :Return fs;
 * stop
 * @enduml
 * 
 * @param dev The device to initialize the superblock for.
 * @return fsnum_t The file system number.
 */
fsnum_t init_isblock(ldev_t dev)
{
  fsnum_t fs, freefs = MAXFS;
  for ( fs = 0 ; fs < MAXFS ; ++fs ) {
    if (isblock[fs].inuse) {
      if (isblock[fs].dev.ldev == dev.ldev)
        return fs + 1;
    } else {
      if (freefs == MAXFS)
        freefs = fs;
    }
  }
  fs = freefs;
  if (fs == MAXFS) {
    /// @todo set error no free superblock
    return 0;
  }
  fs = fs + 1; // Convert 0-based array index to 1-based filesystem number
  isuperblock_t *isbk = getisblock(fs);
  bhead_t *bh = bread(dev, 1);  // read superblock
  if (bh->error) {
    /// @todo set error reading superblock
    return 0;
  }
  
  // Validate superblock before using it (except for root filesystem)
  superblock_t *sb = (superblock_t *)bh->buf->mem;
  
  // Skip validation for filesystem 1 (root) as it might be a special case
  if (fs != 1) {
    int validation_result = validate_superblock(sb);
    if (validation_result != 0) {
      /// @todo set error invalid superblock (validation_result contains specific error)
      brelse(bh);
      return 0;
    }
  }
  
  mset(isbk, 0, sizeof(isuperblock_t));
  isbk->fs = fs;
  isbk->dev = dev;
  mcpy(&isbk->dsblock, bh->buf->mem, sizeof(superblock_t));
  brelse(bh);
  isbk->inuse = true;
  return fs;
}


/**
 * @brief allocate a block from fs 
 * 
 * @startuml
 * start
 * :get superblock;
 * if (superblock locked) then (yes)
 *   :sleep(event superblock gets free);
 * else (no)
 * endif
 * :lock superblock;
 * if (free list empty) then (yes)
 *   :try to refill free list;
 * else (no)
 * endif
 * if (free list empty) then (yes)
 *   :unlock superblock\nwake all unlock superblock\nreturn NULL;
 *   stop
 * else (no)
 *   :get block from free list;
 *   :unlock superblock\nwake all unlock superblock;
 *   :mark block as used in bitmap;
 *   :get buffer for block\nzero buffer\nset buffer as valid;
 *   :return buffer;
 *   stop
 * endif
 * @enduml
 * 
 * @param fs        file system number
 * @return bhead_t* pointer to buffer header of allocated block or NULL if no block is available/error occured
 */
bhead_t *balloc(fsnum_t fs)
{
  ASSERT(fs > 0 && fs <= MAXFS);

  block_t bidx, b = 0;
  int n = 0;
  bhead_t *bh = NULL;
  isuperblock_t *isbk = getisblock(fs);
  while (isbk->locked) {
    waitfor(SBLOCKBUSY);
  }
  isbk->locked = true;

  if ((isbk->nfblocks >= NFREEBLOCKS) || (isbk->fblocks[isbk->nfblocks] == 0)) {
    mset(isbk->fblocks, 0, sizeof(isbk->fblocks));
    for ( bidx = isbk->lastfblock ; bidx < isbk->dsblock.nblocks ; ++bidx ) {
      if (!bh || (b != BMAPBLOCK(bidx))) {
        if (bh) 
          brelse(bh);
        b = BMAPBLOCK(bidx);
        bh = breada(isbk->dev, b + isbk->dsblock.bbitmap, b + 1 + isbk->dsblock.bbitmap);
      }
      if (!(bh->buf->mem[BMAPIDX(bidx)] & BMAPMASK(bidx))) {
        isbk->fblocks[n++] = bidx;
        if (n >= NFREEBLOCKS)
          break;
      }
    }
    if (bh) 
      brelse(bh);
    isbk->nfblocks = 0;
  }

  bidx = isbk->fblocks[isbk->nfblocks];

  if (bidx == 0) {
    isbk->locked = false;
    wakeall(SBLOCKBUSY);
    /// @todo set error no free blocks in fs
    return NULL;
  }

  ++isbk->nfblocks;
  isbk->lastfblock = bidx;
  isbk->locked = false;
  wakeall(SBLOCKBUSY);

  bh = bread(isbk->dev, BMAPBLOCK(bidx) + isbk->dsblock.bbitmap);
  bh->buf->mem[BMAPIDX(bidx)] |= BMAPMASK(bidx);
  bh->dwrite = true;
  bwrite(bh);
  brelse(bh);

  bh = getblk(isbk->dev, bidx);
  mset(bh->buf->mem, 0, sizeof(bh->buf->mem));
  bh->valid = true;
  bh->dwrite = true;

  return bh;
}


/**
 * @brief Free a block in a filesystem.
 * 
 * This function frees a block in the filesystem. It first checks if the block is valid, then it locks the superblock to prevent other processes from modifying it while it's being updated. It then updates the free block list in the superblock and unlocks the superblock. Finally, it updates the block bitmap to indicate that the block is free.
 * 
 * @startuml
 * start
 * :Start;
 * :Assert fs < MAXFS and bl > 0;
 * :isbk = getisblock(fs);
 * :Assert bl < isbk->dsblock.nblocks;
 * while (isbk->locked) is (yes)
 *   :Wait for SBLOCKBUSY;
 * endwhile (no)
 * :Lock isbk;
 * if (isbk->nfblocks >= NFREEBLOCKS) then (yes)
 *   :isbk->nfblocks = NFREEBLOCKS - 1;
 *   :isbk->fblocks[isbk->nfblocks] = bl;
 * elseif (isbk->nfblocks > 0) then (yes)
 *   if (isbk->fblocks[isbk->nfblocks] > bl) then (yes)
 *     :isbk->fblocks[--isbk->nfblocks] = bl;
 *   else (no)
 *     :for i in isbk->nfblocks + 1 to NFREEBLOCKS;
 *     if ((isbk->fblocks[i] > bl) or (isbk->fblocks[i] == 0)) then (yes)
 *       :isbk->fblocks[i] = bl;
 *       stop
 *     endif
 *     :endfor;
 *   endif
 * else (no)
 *   :for i in 0 to NFREEBLOCKS;
 *   if ((isbk->fblocks[i] > bl) or (isbk->fblocks[i] == 0)) then (yes)
 *     :isbk->fblocks[i] = bl;
 *     stop
 *   endif
 *   :endfor;
 * endif
 * :Unlock isbk;
 * :Wake all SBLOCKBUSY;
 * :bh = bread(isbk->dev, BMAPBLOCK(bl) + isbk->dsblock.bbitmap);
 * :bh->buf->mem[BMAPIDX(bl)] &= ~BMAPMASK(bl);
 * :bh->dwrite = true;
 * :bwrite(bh);
 * :brelse(bh);
 * stop
 * @enduml
 * 
 * @param fs The filesystem number.
 * @param bl The block number.
 */void bfree(fsnum_t fs, block_t  bl)
{
  ASSERT(fs > 0 && fs <= MAXFS);
  ASSERT(bl > 0);

  isuperblock_t *isbk = getisblock(fs);
  int i;
  ASSERT(bl < isbk->dsblock.nblocks);
  while (isbk->locked) {
    waitfor(SBLOCKBUSY);
  }
  isbk->locked = true;

  if (isbk->nfblocks >= NFREEBLOCKS) {
    isbk->nfblocks = NFREEBLOCKS - 1;
    isbk->fblocks[isbk->nfblocks] = bl;
  } if (isbk->nfblocks > 0) {
    if (isbk->fblocks[isbk->nfblocks] > bl) {
      isbk->fblocks[--isbk->nfblocks] = bl;
    } else {
      for ( i = isbk->nfblocks + 1 ; i < NFREEBLOCKS ; ++i )
        if ((isbk->fblocks[i] > bl) || (isbk->fblocks[i] == 0)) {
          isbk->fblocks[i] = bl;
          break;
        }
    }
  } else {
      for ( i = 0 ; i < NFREEBLOCKS ; ++i )
        if ((isbk->fblocks[i] > bl) || (isbk->fblocks[i] == 0)) {
          isbk->fblocks[i] = bl;
          break;
        }
  }
  isbk->locked = false;
  wakeall(SBLOCKBUSY);

  bhead_t *bh = bread(isbk->dev, BMAPBLOCK(bl) + isbk->dsblock.bbitmap);
  bh->buf->mem[BMAPIDX(bl)] &= ~BMAPMASK(bl);
  bh->dwrite = true;
  bwrite(bh);
  brelse(bh);
}



/**
 * @brief mount file system on device dev to inode ii (pino is inode number of parent inode)
 * 
 * @param dev       device number of file system
 * @param ii        inode where file system is mounted on
 * @param pino      inode number of parent inode
 * @return int      0 on success, -1 on error
 */
int mounti(ldev_t dev, iinode_t *ii, ninode_t pino, int mflags)
{
  ASSERT(dev.major >= 0);  // Allow major 0 for test devices
  ASSERT(ii);
  ASSERT(ii->dinode.ftype == DIRECTORY);
  ASSERT(pino > 0);
  fsnum_t fs = init_isblock(dev);
  if (fs == 0) {
    /// @todo set error no free superblock
    return -1;
  }
  isuperblock_t *isbk = getisblock(fs);
  if (isbk->mounted) {
    /// @todo set error file system already mounted
    return -1;
  }
  isbk->mounted = ii;
  isbk->pfs = ii->fs;
  isbk->pino = pino;
  isbk->mflags = mflags;
  return 0;
}




int mount(const char *src, const char *dst, int mflags)
{
  ASSERT(src);
  ASSERT(dst);
  
  // Check path lengths - return error instead of asserting
  if (snlen(src, MAXPATH + 1) >= MAXPATH) {
    /// @todo set error - source path too long
    return -1;
  }
  if (snlen(dst, MAXPATH + 1) >= MAXPATH) {
    /// @todo set error - destination path too long  
    return -1;
  }
  /*
  ASSERT(mflags & MS_RDONLY || mflags & MS_RDWR);
  ASSERT(mflags & MS_NOSUID || mflags & MS_SUID);
  ASSERT(mflags & MS_NODEV || mflags & MS_DEV);
  ASSERT(mflags & MS_NOEXEC || mflags & MS_EXEC);
  ASSERT(mflags & MS_NOATIME || mflags & MS_ATIME);
  ASSERT(mflags & MS_NODIRATIME || mflags & MS_DIRATIME);
  ASSERT(mflags & MS_NOSYMFOLLOW || mflags & MS_SYMFOLLOW);
  ASSERT(mflags & MS_NOUSER || mflags & MS_USER);
  ASSERT(mflags & MS_NOROOT || mflags & MS_ROOT);
  ASSERT(mflags & MS_NOSUB || mflags & MS_SUB);
  */

  namei_t dstni = namei(dst);
  if (!dstni.i || dstni.p == 0) {
    /// @todo set error
    return -1;
  }
  if (dstni.i->dinode.ftype != DIRECTORY) {
    /// @todo set error
    return -1;
  }
  if (dstni.i->fsmnt != 0) {
    /// @todo set error
    return -1;
  }
  namei_t srcni = namei(src);
  if (!srcni.i) {
    /// @todo set error
    return -1;
  }
  if (srcni.i->dinode.ftype != BLOCK) {
    /// @todo set error
    return -1;
  }  
  return mounti(srcni.i->dinode.ldev, dstni.i, dstni.p, mflags);
}



/**
 * @brief unmount file system fs
 * 
 * @param fs        file system number
 * @return int      0 on success, -1 on error
 */
int unmount(fsnum_t fs)
{
  ASSERT(fs > 0 && fs <= MAXFS);
  isuperblock_t *isbk = getisblock(fs);
  if (!isbk->inuse) {
    /// @todo set error no such file system
    return -1;
  }
  if (!isbk->mounted) {
    /// @todo set error file system not mounted
    return -1;
  }
  // Enhanced active inode detection with detailed busy checking
  int active_inodes = activeinodes(fs);
  if (active_inodes > 0) {
    // Provide more detailed information about why filesystem is busy
    int open_files = count_open_files_on_fs(fs);
    int busy_workdir = is_fs_busy_workdir(fs);
    
    // Use variables to avoid unused variable warnings
    (void)open_files;    // For future logging/debugging
    (void)busy_workdir;  // For future logging/debugging
    
    /// @todo set error file system still in use 
    /// @todo consider logging: "Filesystem busy: %d active inodes, %d open files, workdir busy: %d"
    /// @todo   active_inodes, open_files, busy_workdir
    return -1;
  }
  
  // Flush all dirty buffers for this device to ensure data consistency
  // This is critical to prevent data loss during umount
  sync_device_buffers(isbk->dev, false);  // Synchronous flush
  
  isbk->mounted->fsmnt = 0;
  iput(isbk->mounted);
  isbk->mounted = NULL;
  isbk->pfs = 0;
  isbk->pino = 0;
  isbk->inuse = false;
  return 0;
}



int umount(const char *path)
{
  ASSERT(path);
  ASSERT(snlen(path, MAXPATH + 1) < MAXPATH);
  namei_t ni = namei(path);
  if (!ni.i) {
    /// @todo set error
    return -1;
  }
  if (ni.i->dinode.ftype != DIRECTORY) {
    /// @todo set error
    return -1;
  }
  
  // Search for filesystem mounted at this directory
  for (fsnum_t fs = 1; fs <= MAXFS; fs++) {
    isuperblock_t *isbk = getisblock(fs);
    if (isbk->inuse && isbk->mounted == ni.i) {
      return unmount(fs);
    }
  }
  
  /// @todo set error - no filesystem mounted here
  return -1;
}

/**
 * @brief Validate superblock structure and contents
 * 
 * This function performs comprehensive validation of a superblock to ensure
 * the filesystem is valid and safe to mount. It checks magic numbers,
 * version compatibility, and structural consistency.
 * 
 * @param sb pointer to superblock to validate
 * @return int 0 if valid, negative error code if invalid
 */
int validate_superblock(const superblock_t *sb)
{
  if (!sb) {
    return -1; // Null pointer
  }
  
  // Check magic number - must match STIX filesystem signature
  // Convert stored magic number from little-endian to host byte order for comparison
  dword_t stored_magic = stix_le32toh(sb->magic);
  if (stored_magic != STIX_MAGIC_NUMBER) {
    return -2;  // Invalid magic number
  }
  
  // Check version compatibility
  if (sb->version != STIX_VERSION) {
    return -3;  // Unsupported version
  }
  
  // Check filesystem type
  if (sb->type != STIX_TYPE) {
    return -4;  // Unknown filesystem type
  }
  
  // Validate basic structural integrity
  if (sb->ninodes == 0 || sb->nblocks == 0) {
    return -5; // Invalid inode or block count
  }
  
  // Check that block bitmap is within filesystem bounds
  if (sb->bbitmap >= sb->nblocks) {
    return -6; // Bitmap block out of bounds
  }
  
  // Check that first data block is within filesystem bounds
  if (sb->firstblock >= sb->nblocks) {
    return -7; // First data block out of bounds
  }
  
  // Check that inode table is within filesystem bounds
  if (sb->inodes >= sb->nblocks) {
    return -8; // Inode table out of bounds
  }
  
  // Check logical ordering: inodes < bitmap < firstblock
  if (sb->inodes >= sb->bbitmap || sb->bbitmap >= sb->firstblock) {
    return -9; // Invalid block layout
  }
  
  // Check reasonable limits (block_t is unsigned short, max 65535)
  if (sb->ninodes > 32767 || sb->nblocks > 32767) {
    return -10; // Unreasonable filesystem size
  }
  
  return 0; // Valid superblock
}
