#!/usr/bin/python

import csv
import os
import sys
from collections import defaultdict


#globals
csvFileName= ""  
super_block = None
group_buffer = []
free_block_list= []
inode_list =[]
dir_entry_list = []
indirect_list = []
first_non_reserved_block = None
reference_dictionary = defaultdict(list)
free_block_set = set()
free_inode_set = set()
error_count = 0


invalid_list = ["", "INDIRECT ","DOUBLE INDIRECT ", "TRIPLE INDIRECT "]

class Reference: #this classes will be used to track whether a block is reference multiple times
	def __init__(self,arg):
		self.block_num = arg[0]
		self.inode_num = arg[1]
		self.offset = arg[2]
		self.level = arg[3]

#for obtaining the class entries, I used the 3A specs
class Super:
	def __init__(self, arg): #arg[0] reserve for keyword SUPERBLOCK
		self.total_block_count = int(arg[1])
		self.total_inode_count = int (arg[2])
		self.block_size = int (arg [3])
		self.inode_size = int (arg [4])
		self.blocks_per_group = int (arg [5])
		self.inode_per_group = int (arg [6])
		self.first_non_res = int (arg [7])
		global first_non_reserved_block
		first_non_reserved_block = int (arg [7])


class Group:
	def __init__(self, arg):
		self.group_num = int (arg[1])
		self.total_group_count = int (arg [2])
		self.total_inode_count = int (arg [3])
		self.free_block_count = int(arg [4])
		self.free_inode_count = int (arg[5]) 
		# we dont need the rest
class FreeBlock:
	def __init__(self, arg):
		self.block_num = int (arg [1])

class FreeInode:
	def __init__(self, arg):
		self.inode_inode = int (arg [1])
class Inode:
	def __init__(self, arg):
		self.inode_number = int (arg[1])
		self.type = arg[2] #we dont change to int since this is a string
		self.owner = int (arg[4])
		self.group = int (arg[5])
		self.link_count = int (arg[6])
		self.file_size = int (arg[10])
		self.num_blocks = int (arg[11])
		#self.inode_block_numbers = int (arg[11:26])
		self.inode_block_numbers = arg[12:27]
		#turn all of the strings into int
		for i in range(0, len(self.inode_block_numbers)):
			self.inode_block_numbers[i] = int (self.inode_block_numbers[i])

class Directory_Entry:
	def __init__(self, arg):
		self.parent_inode_number = int (arg[1])
		self.file_inode_number = int (arg[3])
		self.entry_len = int (arg[4])
		self.name_len = int (arg [5])
		self.file_name = arg[6] #not an int, string


class Indrect:
	def __init__(self, arg):
		self.inode_num_owner = int (arg[1])
		self.level = int (arg[2])
		self.logical_offset = int (arg[3])
		self.block_num = int (arg[4])
		self.block_num_ref_block = int (arg[5])

def handleError(reason):
	sys.stderr.write("Error: %s") % (reason)
	exit(1)


def handleOpenAndCSV(fileName):
	with open(fileName, 'rb') as csvfile: #rb is needed on some cases
		reader = csv.reader(csvfile)
		for row in reader: #each row of csv file needs to be parsed
			if row[0] == "SUPERBLOCK":
				global super_block
				super_block = Super(row) #because we know that we have only one group
			elif row[0] == "GROUP":
				group_buffer.append(Group(row))
			elif row[0] == "BFREE":
				global free_block_list
				free_block_list.append(FreeBlock(row))
				global free_block_set
				free_block_set.add(int(row[1]))
			elif row [0] == "IFREE":
				global free_inode_set
				free_inode_set.add(int(row[1]))
			elif row [0] == "INODE":
				global inode_list
				inode_list.append(Inode(row))
			elif row[0] == "DIRENT":
				global dir_entry_list
				dir_entry_list.append(Directory_Entry(row))
			elif row [0] == "INDIRECT":
				global indirect_list
				indirect_list.append(Indrect(row))
				free_block_set.add(int(row[5]))
			else:
				handleError("The CSV file format was invalid")

def handleInvalidReserved(tag, block_num,inode_num, offset,level):
	global error_count
	
	if tag == "invalid":
		global invalid_list
		print "INVALID %sBLOCK %d IN INODE %d AT OFFSET %d" %(invalid_list[level],block_num, inode_num, offset)
		error_count += 1
	if tag == "reserved":
		print "RESERVED %sBLOCK %d IN INODE %d AT OFFSET %d" %(invalid_list[level],block_num, inode_num, offset)
		error_count += 1

def handleUnreferenced():
	global error_count

	for i in range (8, super_block.total_block_count): #all blocks
		if i not in free_block_set and reference_dictionary.get(i) == None: #block not in free list nor in reference dict
			print "UNREFERENCED BLOCK %d" % (i)
			error_count += 1
def handleAllocatedOnFreeList():
	global error_count

	for i in range (8, super_block.total_block_count): #all blocks
		if i in free_block_set and reference_dictionary.get(i) != None: #block in free list and in reference dict
			print "ALLOCATED BLOCK %d ON FREELIST" % (i)
			error_count += 1

def handleDuplicate():
	global error_count

	for i in range (8, super_block.total_block_count): #all blocks
		if reference_dictionary.get(i) != None and len (reference_dictionary[i]) >= 2:
			#process all of those duplicates
			for ref in reference_dictionary[i]:
				print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d" %(invalid_list[ref.level],ref.block_num, ref.inode_num, ref.offset)
				error_count += 1

def handleBlockConsistency():
	#go through every inode and see if the block number they provide is invalid
	#use the super to find out if the block number is greater than no reserved

	#IMORTANT: we need to read [12:14] to be able to to just get those blocks
	#for inode in inode_list:
	for inode in inode_list:
		for j in range(0,len(inode.inode_block_numbers)):
			global super_block
			block_num = inode.inode_block_numbers[j]
			#check for INVALID
			tag = ""
			if (block_num < 0 or block_num > super_block.total_block_count):
				tag = "invalid"
			#check for reserved
			#elif (block_num < super_block.first_non_res):
			elif(block_num < 8 and block_num != 0): #0 is a valid block number pointer, indicates that we are not pointing to anything
				tag = "reserved"
			if len(tag) != 0:
				if j < 12:
					handleInvalidReserved(tag,block_num, inode.inode_number,j,0)
					continue
				else:
					offset = None
					if j == 12:
						offset = 12
					if j == 13:
						offset = 268
					if j == 14:
						offset = 65804
					handleInvalidReserved (tag,block_num, inode.inode_number,offset,j%11)
					continue
			#now we know that the block numbers (pointers) were valid
			#check if the offset is correct
			if j < 12:
				reference_dictionary[block_num].append(Reference([block_num, inode.inode_number, j,0])) 
			else:
				offset = None
				if j == 12:
					offset = 12
				elif j== 13:
					offset = 268
				elif  j == 14:
					offset = 65804
				reference_dictionary[block_num].append(Reference([block_num, inode.inode_number, offset,j%11]))


	#also check indirect blocks for the block numbers
	for indirect in indirect_list:
		tag = ""
		if indirect.block_num_ref_block < 0 or indirect.block_num_ref_block > super_block.total_block_count:
			tag = "invalid"
		elif indirect.block_num_ref_block < 8 and indirect.block_num_ref_block != 0:
			tag = "reserved"
		if (len(tag)!= 0):
			handleInvalidReserved(tag,indirect.block_num_ref_block,indirect.inode_num_owner, indirect.logical_offset, indirect.level)

	#now we can go through and see which entries are not
	handleUnreferenced()
	handleAllocatedOnFreeList()
	handleDuplicate()

def handleInodeAllocation():
	global error_count

	# determine consistency between allocated/unallocated inodes and 
	# their status on the inode bitmap
	global super_block
	num_inodes = super_block.total_inode_count
	
	for i in range(0, num_inodes):
		# don't need to check reserved inodes other than 2
		# so ignore first 10 inodes
		if i < 11 and i != 2:
			continue

		allocated_flag = False
		free_flag = False

		for inode in inode_list:
			if inode.inode_number == i:
				allocated_flag = True
				break

		if i in free_inode_set:
			free_flag = True

		if allocated_flag == free_flag:
			if free_flag:
				print "ALLOCATED INODE %d ON FREELIST" % (i)
				error_count += 1
			else:
				print "UNALLOCATED INODE %d NOT ON FREELIST" % (i)
				error_count += 1

def handleDirectoryConsistency():
	global error_count

	# check every allocated inode for reference count consistency
	global super_block
	num_inodes = super_block.total_inode_count

	child_parent_dict = {}
	
	for inode in inode_list:
		inode_link_count = 0

		# scan through all directories
		for directory in dir_entry_list:
			# increment inode reference count by 1 if current
			# inode is the parent inode of the directory
			if inode.inode_number == directory.file_inode_number:
				inode_link_count += 1

			# check that the referenced inodes are valid
			elif inode.inode_number == directory.parent_inode_number and (directory.file_inode_number < 1 or directory.file_inode_number > num_inodes):
				print "DIRECTORY INODE %d NAME %s INVALID INODE %d" %(directory.parent_inode_number, directory.file_name, directory.file_inode_number)
				error_count += 1

			# check that the referenced inodes are allocated
			elif inode.inode_number == directory.parent_inode_number and not any(i.inode_number == directory.file_inode_number for i in inode_list):
				print "DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" %(directory.parent_inode_number, directory.file_name, directory.file_inode_number)
				error_count += 1

			# add to the dictionary to check for correctness again later
			if (directory.file_name == "'.'" and directory.file_name == "'..'") or directory.parent_inode_number == 2:
				child_parent_dict[directory.file_inode_number] = directory.parent_inode_number
			

		if inode_link_count != inode.link_count:
			print "INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" % (inode.inode_number, inode_link_count, inode.link_count)
			error_count += 1

	# scan through directories again to check self and parent links
	for directory in dir_entry_list:
		if directory.file_name == "'.'" and directory.file_inode_number != directory.parent_inode_number:
			print "DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d" %(directory.parent_inode_number, directory.file_inode_number, directory.parent_inode_number)
			error_count += 1
		
		elif directory.file_name == "'..'" and directory.file_inode_number != child_parent_dict[directory.parent_inode_number]:
			print "DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" %(directory.parent_inode_number,  directory.file_inode_number, child_parent_dict[directory.parent_inode_number])
			error_count += 1

def main():
	if (len(sys.argv) != 2): #we need the name of the csv file to be passed in as an arg
		sys.stderr.write ("Usage: ./lab3b filename.csv")
		exit(1)
	global csvFileName
	csvFileName = sys.argv[1]
	handleOpenAndCSV(csvFileName)
	handleBlockConsistency()
	handleInodeAllocation()
	handleDirectoryConsistency()

if __name__ == '__main__':
	main()

	if error_count != 0:
		exit(2)
	else:
		exit(0)

