###############################################################################
#
# MODULE:          JN518x RETAIN Map File Analysis
#
# COMPONENT:       JN518x_RETAIN.awk
#
# AUTHOR:          Martin Looker
#
# DESCRIPTION:     Analyses .map file and outputs information on how much 
#                  RAM0 memory is retained during sleep.
#
# USAGE:		   gawk -f JN518x_RETAIN.awk input.map
#
###############################################################################
#
# This software is owned by NXP B.V. and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5169,
# JN5179, JN5189].
# You, and any third parties must reproduce the copyright and warranty notice
# and any other legend of ownership on each copy or partial copy of the
# software.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Copyright NXP B.V. 2018. All rights reserved
#
###############################################################################
# Convert hex string to numeric
function myhextonum(sHex, u32Num, u32Len, u32Pos, sChar, u32Idx, c)
{
	# Set return value
	u32Num = 0
	# Valid hex string ? 
	if (sHex ~ /^0[xX][[:xdigit:]]+$/) {
		# Get just hex digits
		sHex = substr(sHex, 3)
		# Convert to lower case
		sHex = tolower(sHex)
		# Get length
		u32Len = length(sHex)
		# Loop through characters	
		for (u32Pos = 1; u32Pos <= u32Len; u32Pos++) {
			# Get character
			sChar = substr(sHex, u32Pos, 1)
			# Get index (0 returned if not found)
			u32Idx = index("123456789abcdef", sChar)
			# Update number
			u32Num = (u32Num * 16) + u32Idx
		}
	}
	# Reeturn number
    return u32Num
}

# Extract filename from full path
function mypathtofile(sPath, u32PathParts, u32PathPart, asPathSplit, sFile, u32Dot) {
	# Split SubsectionFileFull separators: / \ ( ) 
	u32PathParts = split(sPath, asPathSplit, /\/|\\|\(|\)/)
	# Empty filename
	sFile = ""
	# Get filename from last non-blank folder
	for (u32PathPart = u32PathParts; sFile == "" && u32PathPart > 0; u32PathPart--) {
		sFile = asPathSplit[u32PathPart]
	}
#	u32Dot = index(sFile, ".")
#	if (u32Dot != 0) {
#		sFile = substr(sFile, 1, u32Dot-1)
#	}
	# Return file
	return sFile
}

# Parse and add section to list
function myaddsection() {
	u32ThisLine = NR
	asField[1] = $1
	asField[2] = ""
	asField[3] = ""
	# 4 fields ? 
	if (NF >= 3) {
		asField[2] = $2
		asField[3] = $3
	}
	# Only one field ?
	if (NF == 1) {
		# Parse the next (continuation line)
		getline
		# Note fields from continued line
		asField[2] = $1
		asField[3] = $2
	}
	# Valid data for address and length ?
	if (asField[2] ~ /^0x/ && asField[3] ~ /^0x/) {
		# Note fields from this line
		u32Addr = myhextonum(asField[2])
		u32Size = myhextonum(asField[3])
		# Add as section
		u32Sections++
		asSection[u32Sections] = asField[1]
		au32SectionAddr[u32Sections] = u32Addr
		au32SectionEnd[u32Sections]  = au32SectionAddr[u32Sections] + u32Size
		# Loop through banks
		for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
			# Does this section start in this bank ? 
			if (au32SectionAddr[u32Sections] >= au32BankAddr[u32Bank] &&
				au32SectionAddr[u32Sections] <  au32BankAddr[u32Bank] + au32BankSize[u32Bank]) {
				au32SectionBankStart[u32Sections] = u32Bank
			}
			# Does this section end in this bank ? 
			if (au32SectionEnd[u32Sections] >= au32BankAddr[u32Bank] &&
				au32SectionEnd[u32Sections] <  au32BankAddr[u32Bank] + au32BankSize[u32Bank]) {
				au32SectionBankEnd[u32Sections] = u32Bank
			}
		}
	}
}

# Pre-processing
BEGIN {
	bProcess            = 0  # Current part of file being processed
	u32Banks            = 0  # Number of memory banks
	u32Sections         = 0  # Number of sections
	
	# Create memory banks
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK0_16KB"
	au32BankAddr[u32Banks] = myhextonum("0x04000000")
	au32BankSize[u32Banks] = 16 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK1_16KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 16 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK2_16KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 16 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK3_16KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 16 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK4_8KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 8 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK5_8KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 8 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK6_4KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 4 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	u32Banks++	
	asBank[u32Banks]       = "RAM0_BANK7_4KB"
	au32BankAddr[u32Banks] = au32BankAddr[u32Banks-1] + au32BankSize[u32Banks-1]
	au32BankSize[u32Banks] = 4 * 1024
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
}

# Record begins "Linker script and memory map"
/^Linker script and memory map/ {
	# Start processing data from here 
	bProcess = 1
}

# Record begins "OUTPUT(" - end of memory map
/^OUTPUT\(/ {
	# Stop processing data from here
	bProcess = 0
}

# Section detection
((/^\.heap/               && $1 == ".heap")) {
	if (bProcess) {
		# Add as new section 
		myaddsection()
	}
}

# Section detection
((/^\.data/               && $1 == ".data")) {
	if (bProcess) {
		# Add as new section 
		myaddsection()
	}
}

# Post-processing END
END {
	# Get filename
	sFileName = mypathtofile(FILENAME)
	# Get path without extension
	sPathNoExt = substr(FILENAME, 1, index(FILENAME, ".map"))
	# Build output file paths
	sPathTxt  = sPathNoExt "retain.txt"
	# Should have two sections
	if (u32Sections == 2) {
		# First section found was heap ? 
		if (asSection[1] == ".heap") {
			# Calculate retained memory from start of heap to end of last bank
			u32Retain = au32BankAddr[u32Banks] + au32BankSize[u32Banks] - au32SectionAddr[1]
			# Calculate spare from start of bank to start of heap
			u32Spare = au32SectionAddr[1] - au32BankAddr[au32SectionBankStart[1]]
			# Note heap address 
			u32SectionAddr = au32SectionAddr[1]
			# Note we found the heap
			bHeap = 1
			# Loop through retained banks
			for (u32Bank = au32SectionBankStart[1]; u32Bank <= u32Banks; u32Bank++) {
				# Flag bank is retained
				abBankRetain[u32Bank] = 1
			}
		}
		# Didn't find heap first ?
		else {
			# Calculate retained memory from start of memory to end of heap
			u32Retain = au32SectionEnd[2] - au32BankAddr[1]
			# Calculate spare from end of bank to end of heap
			u32Spare = au32BankEnd[au32SectionBankEnd[2]] - au32SectionEnd[2]
			# Note heap address 
			u32SectionAddr = au32SectionEnd[2]
			# Note we found the heap
			bHeap = 1
			# Loop through retained banks
			for (u32Bank = 1; u32Bank <= au32SectionBankEnd[2]; u32Bank++) {
				# Flag bank is retained
				abBankRetain[u32Bank] = 1
			}
		}
		# Bank 7 is always retained
		abBankRetain[8] = 1
		# Make a clear string
		sBanks = ""
		# Loop through banks
		for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
			# Bank is retained ?
			if (abBankRetain[u32Bank]) {
				# Update total size
				u32RetainTotal += au32BankSize[u32Bank]
				# Update string
				sBanks = sBanks (u32Bank-1)
			}
			# Bank not retained ?
			else {
				# Update string
				sBanks = sBanks "-"
			}
		}
		# Calculate total in kb
		printf "%7s %7s %7s %15s %s\n", "retain", "spare", "kb", "banks", "filename" > sPathTxt
		printf "%7d %7d %7d %15s %s\n", u32Retain, u32Spare, (u32RetainTotal/1024), sBanks, FILENAME > sPathTxt
	}
	# Didn't find enough sections ?
	else {
		# Output error
		printf "JN518x_RETAIN.awk ERROR: .heap or .data sections not found!\n" > sPathTxt
	}
}