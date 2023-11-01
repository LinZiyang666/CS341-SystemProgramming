/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

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

    return -1;
}

// HELPERS

// use offset of parent_inode->indirect to get to the start of `Indirect Data Blocks` region
data_block_number *get_indirects(file_system *fs, inode *parent_node)
{
    data_block_number *indirect_blocks = (data_block_number *)(fs->data_root + parent_node->indirect); 
    return indirect_blocks;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path)
{
    // Land ahoy!

    inode *node = get_inode(fs, path);
    if (node) // inode already exists
        return NULL;
    // the path is NOT a valid pathname
    if (1 != valid_filename(path))
        return NULL;
    // inode cannot be created

    inode_number new_node_idx = first_unused_inode(fs);
    if (-1 == new_node_idx)
        return NULL;

    // find its parent inode
    const char *filename;
    inode *parent_inode = parent_directory(fs, path, &filename);
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
        {                                                                                                 // the last data block is an Indirect Block
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
            new_db_idx = add_data_block_to_inode(fs, parent_inode);
            if (new_db_idx == -1) // FULL
                return NULL;
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
    src.name = filename;
    make_string_from_dirent(req_data_block, src);
    //increase the parent's size.
    parent_inode->size += FILE_NAME_ENTRY; // The number of bytes written by calling make_string_from_dirent will be equal to FILE_NAME_ENTRY as defined in minixfs.h
    return new_node;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off)
{
    if (!strcmp(path, "info"))
    {
        // TODO implement the "info" virtual file here
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off)
{
    // X marks the spot
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off)
{
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    return -1;
}
