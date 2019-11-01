#!/usr/bin/python

import csv
import os
import sys
from collection import defaultDict

#global variables
csv = ""
super_block = None
group = []
blocks = []
inodes  = []
dir_ents = []
indir_ents = []

unused_block = None
err = None 
dir_references = defaultDict(list)

inodes_set = set()
blocks_set = set()

invalid_tags = ["","INDIRECT ","DOUBLE INDIRECT ","TRIPLE INDIRECT " ] 

class tree:
    def __init__(self, arg):
        self.block_num = arg[0]
        self.inode_num = arg[1]
        self.offset = arg[2]
        self.level = arg[3]

class super_group:
    def __init__(self, arg):
        self.total_block_count = int(arg[1])
        self.total_inode_count = int(arg[2])
        self.block_size = int(arg[3])
        self.inode_size = int(arg[4])
        self.blocks_per_group = int(arg[5])
        self.inodes_per_group = int(arg[6])
        global unused_block
        unused_block  = int(arg[7])

class group:
    def __init__(self, arg):
        self.group_num = int(arg[1])
        self.total_group_count = int(arg[2])
        self.total_inode_count = int(arg[3])
        self.free_block_count = int(arg[4])
        self.free_inode_count = int(arg[5])
class free_block:
    def __init__(self, arg):
        self.block_num = int(arg[1])
class free_inode:
    def __init__(self, arg):
        self.inode_number = int(arg[1])
        self.type = arg[2]
        self.owner = int(arg[4])
        self.group = int(arg[5])
        
class inode:
    def __init__(self,arg):
        self.inode_number = int(arg[1])
        self.type = arg[2]
        self.owner = int(arg[4])
        self.group = int(arg[5])
        self.link_count = int(arg[6])
        self.file_size = int(arg[10])
        self.num_blocks = int (arg[11])
        self.inode_block_numbers = arg[12:27]

        for i in range(0, len(self.inode_block_numbers)):
            self.inode_block_numbers[i] = int (self.inode_block_numbers[i])
class dir_ent:
    def __init__(self, arg):
        self.parent_inode_number = int(arg[1])
        self.file_inode_number = int(arg[3])
        self.entry_len = int(arg[4])
        self.file_name = arg[6]
class indir_ent:
    def __init__(self, arg):
        self.inode_num_owner = int(arg[1])
        self.level = int(arg[2])
        self.logical_offset = int(arg[3])
        self.block_num = int(arg[4])
        self.block_num_ref_block = int(arg[5])

def errorHandler(err):
        sys.stderr.write("Error: %s") %(reason)
        exit(1)
    
def csv_file(name):
    with open(name, 'rb') as csvfile: #rb required to not corrupt FS
        reader = csv.reader(csvfile)
        for row in reader:
            if row[0] == "SUPER_BLOCK":
                global super_block
                super_block = Super(row)
            elif row[0] == "GROUP":
                group.append(Group(row))
            elif row[0] == "BFREE":
                global blocks 
                blocks.append(FreeBlock(row))
                global blocks_set
                blocks_set.add(int(row[1]))
            elif row[0] == "IFREE":
                global inodes_set
                inodes_set.add(int(row[1]))
            elif row[0] == "INODE":
                global inodes
                inondes.append(Inode(row))
            elif row[0] == "DIRENT":
                global dir_references
                dir_references.append(Directory_Entry(row))
            elif row[0] == "INDIRECT":
                global indir_ents
                indir_ents.append(Indirect(row))
                block_set.add(int(row[5]))
            else:
                errorHandler("CSV File provided is incorrect")
                
def invalidReservation(tag, block, inode, offset, lvl):
        global err
        if tag == 'invalid':
            global indir_ents
            print "INVALID %sBLOCK %d in INODE %d AT OFFSET %d"%(indir_ents[lvl], block, inode, offset)
            err += 1
        if tag == 'reserved':
            print "RESERVED %s BLOCK %d in INODE %d AT OFFSET %d"%(indir_ents[lvl], blocks, inode, offset)
            err+=1
def lru():
    global err
    for i in range (8, super_block.total_block_count): # all blocks
        if i not in block_set and dir_references.get(i) == None: #see if it is referneced
# block neither referenced nor freed. BAD
            print "UNREFERENCED BLOCK %d" % (i)
            err += 1
def freed_block():
# block that belongs to set and free list
    for i in range (8, super_lbock.total_block_count):
        if i in inode_set and dir_references.get(i) !- None: 
            print "UNREFERENCED BLOCK %d IN INODE %d AT OFFSET %d"%(indir_ents[lvl], blocks, inode, offset)
            err+=1
def freelist_pointer():
    global err
    for i in range(8, super_block.total_block_count):
        if i in block_set and dir_references.get(i) != None 


def duplicates():
    global err
            #see how many pointers to same block
            for pointer in dir_references[i]:
                print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"% (indir_ents[pointer.level], pointer.block, pointer.-inode, pointer.offset)and len (dir_references[i]): #block belongs to free list and pointed to in ref dict
                err +=1
                
def valid_block():
    # if i node > 14 then invalid FS, if we go into inode bitmap if the bit points to block but not allocated
    for i in inodes:
        for j in range(0, len(i.inode_block_numbers)):
            global super_block
            descent = i.inode_block_numbers[j]
            #check for invalid by checking if block num < 8 first block (means danging refernce)
            tag = ""
            if(descent < 8 and descent != 0)
                tag = "reserved"
            elif(descent < 8 or descent > super_block.total_block_count)
                if len(tag) !=0:
                    if j < 12:
                        invalidReservation(tag, descent, i.inode_number, offset, j%11)
                        continue
            else:
                offset = None
                if j ==12:
                    offset = 12
                elif j == 13:
                    offset = 268
                elif j == 14:
                    offset = 65804
            #check offset
            if j < 12:
                dir_references[desecent].append(tree([descent, i.inode_number, offset, j%11]))
            else:
                offset = None
                if j == 12:
                    offset = 12
                else if j == 13:
                    offset = 268
                else if j == 14:
                    offset = 65804
                dir_references[descent].append(tree(descent, i.inode_number, offset, j%11]))


        for indirect in inodes:
            tag = ""
            if indirect.block_num_ref_block < 0 or indirect.block_num_ref_block > 0 super_group.total_block_count:
                tag = "";
                if indirect.block_num_ref_block < 8 and indirect.block_num_ref_block != 0:
                    tag = "reserved"
                elif indirect.block_num_ref_block < 0 or indirect.block_num_ref_block > super_group.total_block_count:
                    tag = "invalid"
                if(len(tag) != 0):
                    dir_references(tag, indirect.block_num_ref, indirect.inode_num_owner, indirect.logical_offset, indirect.lvl)
                lru()
                free_block()
                duplicates()

def new_inode():
    global err
    
    global super_group
    inode_count = super_group.total_inode_count
    
    for i in range(0, inode_count):
        if i < 11 and i !=2:
            continue
        free = False
        allocated = False

        for j in inodes:
            if j.inode_number == 1:
                allocated = True
                break

        if i in inodes_set:
            free = True
        if i in allocated == free:
            if free:
                print "ALLOCATED INODE %d ON FREELIST" %(i)
                err +=1
            else:
                print "UNALLOCATED INODE %d NOT ON FREELIST"%(i)
                err += 1

def valid_dir():
    global err
    
                
    
            
        
        
                
                    
                
        
           
                        
