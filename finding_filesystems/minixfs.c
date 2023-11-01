/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks)
{
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

// HELPERS
size_t get_min(size_t x, size_t y)
{
    if (x < y)
        return x;
    else
        return y;
}

// use offset of parent_inode->indirect to get to the start of `Indirect Data Blocks` region
data_block_number *get_indirects(file_system *fs, inode *parent_node)
{
    data_block_number *indirect_blocks = (data_block_number *)(fs->data_root + parent_node->indirect);
    return indirect_blocks;
}

int minixfs_chmod(file_system *fs, char *path, int new_permissions)
{
    // Thar she blows!

    inode *inode = get_inode(fs, path);
    if (!inode) // does not exist
    {
        errno = ENOENT;
        return -1;
    }
    int type = inode->mode >> RWX_BITS_NUMBER;
    inode->mode = (new_permissions | (type << RWX_BITS_NUMBER));

    // This function should update the node's ctim
    clock_gettime(CLOCK_REALTIME, &inode->ctim);
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group)
{
    // Land ahoy!
    inode *inode = get_inode(fs, path);
    if (!inode) // does not exist
    {
        errno = ENOENT;
        return -1;
    }

    if (owner != ((uid_t)-1))
    {
        inode->uid = owner;
    }
    if (group != ((gid_t)-1))
    {
        inode->gid = group;
    }

    // This function should update the node's ctim
    clock_gettime(CLOCK_REALTIME, &inode->ctim);
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path)
{

    // Land ahoy!

    inode *node = get_inode(fs, path);
    if (node) // inode already exists
        return NULL;
    // inode cannot be created

    inode_number new_node_idx = first_unused_inode(fs);
    if (-1 == new_node_idx)
        return NULL;

    // find its parent inode
    const char *filename;
    inode *parent_inode = parent_directory(fs, path, &filename);
    // the path is NOT a valid pathname
    if (1 != valid_filename(filename))
        return NULL;

    data_block_number num_blocks = (parent_inode->size / sizeof(data_block)); // fully-occupied blocks
    size_t rem = (parent_inode->size % sizeof(data_block));

    if (num_blocks >= NUM_DIRECT_BLOCKS && parent_inode->indirect == UNASSIGNED_NODE)
    { // if all Data Blocks are used & no indirect block used
        int try_add_indirect = add_single_indirect_block(fs, parent_inode);
        if (-1 == try_add_indirect) // FULL
            return NULL;
    }

    // Initialise the `new_node` inode;
    // Compute new_node based on `new_node_idx` = index of first unused inode
    inode *new_node = fs->inode_root + new_node_idx;
    init_inode(parent_inode, new_node);

    data_block *req_data_block = NULL;

    if (rem != 0)
    {
        // Data Block is not full yet

        if (parent_inode->indirect == UNASSIGNED_NODE)
        { // the last data block is a Direct Block -> retrieve it directly
            req_data_block = fs->data_root + parent_inode->direct[num_blocks];
        }
        else
        { // the last data block is an Indirect Block
            data_block_number *indirects = get_indirects(fs, parent_inode);
            req_data_block = fs->data_root + indirects[num_blocks - NUM_DIRECT_BLOCKS];
        }
    }
    else
    { // All data blocks are full -> Have to allocate a new Data Block to the inode

        // 2 cases:
        // a.) add Data Block as a Direct, if we have not assigned indirect yet: parent_inode->indirect == UNASSIGNED_NODE
        // b.) add Data Block as an Indirect Data Block

        data_block_number new_db_idx;
        if (parent_inode->indirect == UNASSIGNED_NODE)
        {
            new_db_idx = add_data_block_to_inode(fs, parent_inode);
            if (new_db_idx == -1) // FULL
                return NULL;
        }
        else
        {
            data_block_number *indirects = get_indirects(fs, parent_inode);
            new_db_idx = add_data_block_to_indirect_block(fs, indirects);
            if (new_db_idx == -1)
                return NULL;
        }

        req_data_block = fs->data_root + new_db_idx;
    }

    // add a new dirent to the parent
    minixfs_dirent src;
    src.inode_num = new_node_idx;
    src.name = strdup(filename); // have to convert `const char *` to `char *`
    make_string_from_dirent(req_data_block->data, src);
    free(src.name);

    // increase the parent's size.
    parent_inode->size += FILE_NAME_ENTRY; // The number of bytes written by calling make_string_from_dirent will be equal to FILE_NAME_ENTRY as defined in minixfs.h
    return new_node;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off)
{
    if (!strcmp(path, "info"))
    {

        // TODO implement the "info" virtual file here
        size_t blocks_used = 0;
        superblock *superblock = fs->meta;
        char *data_map = GET_DATA_MAP(superblock);
        uint64_t nblocks = superblock->dblock_count; // Multiples of 64 -> uint64_t
        for (uint64_t b_idx = 0; b_idx < nblocks; ++b_idx)
        {
            if (data_map[b_idx] == 1) // in use
                blocks_used++;
        }

        char *block_info_str = block_info_string(blocks_used);
        size_t to_read = get_min(strlen(block_info_str) - *off, count);
        memcpy(buf, block_info_str + *off, to_read);
        *off += to_read;

        size_t bytes_read = to_read;
        return bytes_read;
    }

    errno = ENOENT;
    return -1;
}

int get_ceil(int quotient, int dividend)
{
    return (quotient / dividend + (quotient % dividend != 0));
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off)
{
    // X marks the spot
    size_t MAX_SIZE = (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS) * sizeof(data_block);
    if (*off + count > MAX_SIZE)
    {
        errno = ENOSPC;
        return -1;
    }

    inode *node = get_inode(fs, path);
    if (!node)
    {
        node = minixfs_create_inode_for_path(fs, path);
    }

    size_t blocks_required = get_ceil(*off + count, sizeof(data_block));

    if (-1 == minixfs_min_blockcount(fs, path, blocks_required))
    { // not enough blocks & cannot allocate new block
        errno = ENOSPC;
        return -1;
    }

    size_t b_idx = (*off / sizeof(data_block));
    size_t b_off = (*off % sizeof(data_block));

    size_t bytes_wrote = 0;
    while (bytes_wrote < count)
    {

        data_block blk;
        if (b_idx < NUM_DIRECT_BLOCKS)
        { // read Direct Data Block
            data_block_number direct_block_id = node->direct[b_idx];
            blk = fs->data_root[direct_block_id];
        }
        else
        { // read Indirect Data Block
            data_block_number *indirects = get_indirects(fs, node);
            data_block_number data_block_id = indirects[b_idx - NUM_DIRECT_BLOCKS]; // NOTE: data block that is being referenced from the Indirect block
            blk = fs->data_root[data_block_id];
        }

        size_t rem_to_write = sizeof(data_block) - b_off;
        size_t to_write = get_min(rem_to_write, count - bytes_wrote);
        // Perform write
        memcpy(blk.data + b_off, buf + bytes_wrote, to_write);

        bytes_wrote += to_write;
        b_off = 0;
        b_idx++;
    }

    *off += count;
    // If the file size has changed because of the write, the node's size should be updated.
    node->size = *off;

    // This function should update the node's mtim and atim
    clock_gettime(CLOCK_REALTIME, &node->atim);
    clock_gettime(CLOCK_REALTIME, &node->mtim);
    return bytes_wrote;
}

void *get_block(file_system *fs, inode *node, size_t b_idx)
{
    data_block_number* data_block_ptr;
    if (b_idx < NUM_DIRECT_BLOCKS)
    { // get start of Direct Data Block
        data_block_ptr = node->direct;
    }
    else
    { // get start of Indirect Data Block
        data_block_number *indirects = get_indirects(fs, node);
        data_block_ptr = indirects; 
        b_idx -= NUM_DIRECT_BLOCKS; // NOTE: data block that is being referenced from the Indirect block
    }
    void *blk = (void*) (fs->data_root + data_block_ptr[b_idx]); // relative to `data_root`, get to the start of the block `blk` we want to read from, based on the `b_idx` 
    return blk;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off)
{
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);

    inode *node = get_inode(fs, path);
    if (!node)
    {
        errno = ENOENT;
        return -1;
    }
    if ((uint64_t)*off >= node->size) // no bytes to read, we are at the end of the file
        return 0;

    count = get_min(count, node->size - *off); // ensure you don't overflow the file -> read only until the end `node->size` bytes

    uint64_t b_idx = (*off / sizeof(data_block));
    size_t b_off = (*off % sizeof(data_block));

    void *blk = get_block(fs, node, b_idx) + b_off; // Retrieve the starting position where we wanna read in the block
    size_t rem_to_read = sizeof(data_block) - b_off;
    size_t to_read = get_min(rem_to_read, count);

    memcpy(buf, blk, to_read);   // Read `to_read` bytes from the (offsetted) block to the buffer
    size_t bytes_read = to_read; // we have already read `to_read` bytes
    *off += to_read;             // advance offset in FILE
    b_idx++;                     // go to the next block

    while (bytes_read < count)
    {
        blk = get_block(fs, node, b_idx); // No more offset in the block
        size_t to_read = get_min(sizeof(data_block), count - bytes_read);
        memcpy(buf + bytes_read, blk, to_read); // read `to_read` bytes
        bytes_read += to_read;
        *off += to_read;
        b_idx++;
    }

    clock_gettime(CLOCK_REALTIME, &node->atim); // atim is access time, which is the time of last access or the last time a file was read(2).
    return bytes_read;
}