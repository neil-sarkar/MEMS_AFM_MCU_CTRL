import csv
import cog
import urllib.request
import time
import os
# Code Valet
# Automatically generate code for mundane things

class CodeValet:
	def __init__ (self):
		# Check if the latest version of spec csvs have been downloaded yet.
		# Assuming that download one file and cog writing will not take more than 6 seconds to complete
		# If the file is modified more than 6 seconds ago, download again
		
		self.taps_hword = csv.DictReader(open("taps_hword.csv")) 

	# For afm.h
	def firRic1(self):
		file_name = "firRic1.s"
		cog.outl("; Automatically Generated Code")
		line_item_count = 10
		for row in self.taps_hword:
			if line_item_count >= 9:
				cog.out("\n.hword ")
				line_item_count = 0
			else:
				cog.out(",")
			cog.out(" 0x"+row['hex'])
			line_item_count += 1

				
