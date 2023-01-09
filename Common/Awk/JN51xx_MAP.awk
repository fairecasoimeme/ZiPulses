###############################################################################
#
# MODULE:          JN518x RAM0 Map File Analysis
#
# COMPONENT:       JN518x_RAM0.awk
#
# AUTHOR:          Martin Looker
#
# DESCRIPTION:     Analyses .map file and outputs .html containing analysis	of
#                  RAM0 memory.
#
# USAGE:		   gawk -f JN51xx_MAP.awk input.map > output.html
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
# Pre-processing
BEGIN {
	# Debug
	bDebug = 0
	if (bDebug) printf "<!-- BEGIN: -->\n"
	if (bDebug) printf "<!------ gawk-version = '%s' -->\n", PROCINFO["version"]
	# Gawk 3.1.7 ?
	if (PROCINFO["version"] == "3.1.7") {
		# Set record separator
		RS = "\r\n"
	}
	# Processing beginning
	sProcess = "Begin"
	if (bDebug) printf "<!------ sProcess = '%s' -->\n", sProcess
	# Output settings
	bOutputText = 1
	bOutputXml = 0
	bOutputHtml = 1
	bOutputSvg  = 1
	bOutputIds  = 0
}

# Record begins "Source Files" ?
/^Source Files/ {
	if (bDebug) printf "<!-- Source (#%d): '%s' -->\n", FNR, $0
	# Process source files 
	sProcess = "Source"
	if (bDebug) printf "<!------ sProcess = '%s' -->\n", sProcess
}

# Source file line ?
(/^\.\./) {
	# Processing source files ?
	if (sProcess == "Source") {
		if (bDebug) printf "<!-- Source File (#%d): '%s' -->\n", FNR, $0
		# Attempt to add source file
		parseSourceFile()
	}
}

# Parse source file
function parseSourceFile() {
	if (bDebug) printf "<!-- parseSourceFile() NF=%d: -->\n", NF
	# Two fields ? 
	if (NF == 2) {
		# Add source file
		addSourceFile($1, $2)
	}
}

# Add source
function addSourceFile(sSourceOutput, sSourceInput, u32SplitOutput, asSplitOutput, u32SplitInput, asSplitInput, u32Part, sType) {
	if (bDebug) printf "<!-- addSourceFile('%s', '%s') -->\n", sSourceOutput, sSourceInput
	# Split the paths
	u32SplitOutput = split(sSourceOutput, asSplitOutput, /\//)
	u32SplitInput = split(sSourceInput, asSplitInput, /\//)
	sType = "OTH"
	# Loop through input parts (ignoring filename part)
	for (u32Part = 1; u32Part < u32SplitOutput && sType == "OTH"; u32Part++) {
		# Both parts are .. ?
		if (asSplitInput[u32Part] == ".." && asSplitOutput[u32Part] == "..") {
			# Do nothing
		}
		# Input part is .. but output part is not ? 
		else if (asSplitInput[u32Part] == ".." && asSplitOutput[u32Part] != "..") {
			# This is an SDK folder
			sType = "SDK"
		}
		# Something else ?
		else {
			# This is an APP folder
			sType = "APP"
		}
	}
	u32SourceFiles++
	asSourceFileOutput[u32SourceFiles] = sSourceOutput
	asSourceFileInput[u32SourceFiles] = sSourceInput
	asSourceFileType[u32SourceFiles] = sType
}

# Record begins "Archive member included to satisfy reference by file" ?
/^Archive member included to satisfy reference by file/ {
	if (bDebug) printf "<!-- Map Start (#%d): '%s' -->\n", FNR, $0
	# Process return to begin - so we don't process anything
	sProcess = "Begin"
	if (bDebug) printf "<!------ sProcess = '%s' -->\n", sProcess
}

# Record begins "Memory Configuration" ?
/^Memory Configuration/ {
	if (bDebug) printf "<!-- Memory Configuration (#%d): '%s' -->\n", FNR, $0
	# Process memories 
	sProcess = "Memory"
	if (bDebug) printf "<!------ sProcess = '%s' -->\n", sProcess
}

# Memory configuration line ?
(/^flash/ || /^ram/ || /^Flash/ || /^RAM/) {
	# Processing memories ?
	if (sProcess == "Memory") {
		if (bDebug) printf "<!-- Memory (#%d): '%s' -->\n", FNR, $0
		# Attempt to add memory
		parseMemory()
	}
}

# Parse memory 
function parseMemory() {
	if (bDebug) printf "<!-- parseMemory() NF=%d: -->\n", NF
	# Note line
	u32ThisLine = FNR
	# At least three fields ? 
	if (NF >= 3) {
		# Hex values for fields 2 and 3 ?
		if ($2 ~ /^0x/ && $3 ~ /^0x/) {
			u32ThisAddr = hexToNum($2)
			u32ThisSize = hexToNum($3)
			if (bDebug) printf "<!------ u32ThisAddr=0x%08x, u32ThisSize=0x%08x -->\n", u32ThisAddr, u32ThisSize
			# Add memory
			addMemory($1, u32ThisAddr, u32ThisSize, u32ThisLine)
		}
	}
}

# Add memory 
function addMemory(sAddName, u32AddAddr, u32AddSize, u32AddLine) {
	if (bDebug) printf "<!-- addMemory('%s', 0x%08x, 0x%08x, %d): -->\n", sAddName, u32AddAddr, u32AddSize, u32AddLine
	u32Mems++
	asMem[u32Mems]       = sAddName
	asMemHtml[u32Mems]   = "HTML-MEM-" u32Mems
	au32MemAddr[u32Mems] = u32AddAddr
	au32MemSize[u32Mems] = u32AddSize
	au32MemEnd[u32Mems]  = au32MemAddr[u32Mems] + au32MemSize[u32Mems]
	au32MemLine[u32Mems] = u32AddLine
	au32MemUsed[u32Mems] = 0
	# 6x/7x Flash ?
	if (asMem[u32Mems] == "flash") {
		# Add Banks
		addBank("ACTIVE",   au32MemAddr[u32Mems],  (au32MemSize[u32Mems] / 2), u32AddLine)
		addBank("INACTIVE", au32BankEnd[u32Banks], (au32MemSize[u32Mems] / 2), u32AddLine)
	}
	# 6x/7x Ram ?
	else if (asMem[u32Mems] == "ram") {
		# Space reserved for boot code use (0c4c on 6x/7x) ? 
		if (au32MemAddr[u32Mems] == hexToNum("0x0400004c") ||
			au32MemAddr[u32Mems] == hexToNum("0x2000004c")) {
			# Set bootloader size
			u32Bootloader = hexToNum("0x4c")
			# Include the 0x4c reserved for boot code use
			au32MemAddr[u32Mems] -= u32Bootloader
			au32MemSize[u32Mems] += u32Bootloader
			au32MemEnd[u32Mems]  = au32MemAddr[u32Mems] + au32MemSize[u32Mems]
		}
		# Add Banks
		addBank("BANK0", au32MemAddr[u32Mems], au32MemSize[u32Mems], u32AddLine)
	}
	# 8x Flash ?
	else if (asMem[u32Mems] == "Flash640") {
		# Add Banks
		addBank("ACTIVE",   au32MemAddr[u32Mems],    ((1152 / 2) * 512), u32AddLine)
		addBank("INACTIVE", au32BankEnd[u32Banks], ((1152 / 2) * 512),   u32AddLine)
		addBank("PDM",      au32BankEnd[u32Banks], ( 63        * 512),   u32AddLine)
		addBank("RESERVED", au32BankEnd[u32Banks], ( 65        *512),    u32AddLine)
	}
	# 8x RAM0 ?
	else if (asMem[u32Mems] == "RAM0") {
		# Space reserved for boot code (0x400) on 8x ?
		if (au32MemAddr[u32Mems] == hexToNum("0x04000400")) {
			# Note size of bootloader section
			u32Bootloader = hexToNum("0x400")
			# Include the 0x400 reserved for boot code use
			au32MemAddr[u32Mems] -= u32Bootloader
			au32MemSize[u32Mems] += u32Bootloader
			au32MemEnd[u32Mems]  = au32MemAddr[u32Mems] + au32MemSize[u32Mems]
		}
		# Add Banks
		addBank("BANK0", au32MemAddr[u32Mems],  (1024*16), u32AddLine)
		addBank("BANK1", au32BankEnd[u32Banks], (1024*16), u32AddLine)
		addBank("BANK2", au32BankEnd[u32Banks], (1024*16), u32AddLine)
		addBank("BANK3", au32BankEnd[u32Banks], (1024*16), u32AddLine)
		addBank("BANK4", au32BankEnd[u32Banks], (1024*8 ), u32AddLine)
		addBank("BANK5", au32BankEnd[u32Banks], (1024*8 ), u32AddLine)
		addBank("BANK6", au32BankEnd[u32Banks], (1024*4 ), u32AddLine)
		addBank("BANK7", au32BankEnd[u32Banks], (1024*4 ), u32AddLine)
	}
}

# Add bank
function addBank(sAddName, u32AddAddr, u32AddSize, u32AddLine) {
	if (bDebug) printf "<!-- addBank('%s', 0x%08x, 0x%08x, %d): -->\n", sAddName, u32AddAddr, u32AddSize, u32AddLine
	u32Banks++
	au32BankAddr[u32Banks] = u32AddAddr
	au32BankSize[u32Banks] = u32AddSize
	au32BankEnd[u32Banks]  = au32BankAddr[u32Banks] + au32BankSize[u32Banks]
	asBank[u32Banks]       = sAddName "_" (au32BankSize[u32Banks]/1024) "KB"
	asBankHtml[u32Banks]   = "HTML-BANK-" u32Banks
	au32BankLine[u32Banks] = u32AddLine
	au32BankUsed[u32Banks] = 0
	au32BankMem[u32Banks]  = u32Mems
	if (au32MemBank[u32Mems] == 0) au32MemBank[u32Mems] = u32Banks
}

# Record begins "Linker script and memory map"
/^Linker script and memory map/ {
	if (bDebug) printf "<!-- Linker script and memory map (#%d): '%s' -->\n", FNR, $0
	# Process memories 
	sProcess = "Map"
	if (bDebug) printf "<!------ sProcess = '%s' -->\n", sProcess
}

# Section detection
((/^\.bir/                && $1 == ".bir")               ||
 (/^\.flashheader/        && $1 == ".flashheader")       || 
 (/^\.vectors/            && $1 == ".vectors")           || 
 (/^\.exceptions/         && $1 == ".exceptions")        || 
 (/^\.headerA/            && $1 == ".headerA")           ||
 (/^\.ro_nonce/           && $1 == ".ro_nonce")          ||
 (/^\.ro_mac_address/     && $1 == ".ro_mac_address")    ||
 (/^\.ro_ota_header/      && $1 == ".ro_ota_header")     ||
 (/^\.ro_se_lnkKey/       && $1 == ".ro_se_lnkKey")      ||
 (/^\.rodata/             && $1 == ".rodata")            ||
 (/^\.headerB/            && $1 == ".headerB")           ||
 (/^\.filler/             && $1 == ".filler")            ||
 (/^\.ARM.extab/          && $1 == ".ARM.extab")         ||
 (/^\.ARM.exidx/          && $1 == ".ARM.exidx")         ||
 (/^\.text/               && $1 == ".text")              ||
 (/^\.data_discard/       && $1 == ".data_discard")      ||
#(/^\.fillerB/            && $1 == ".fillerB")           ||
 (/^\.bss_discard/        && $1 == ".bss_discard")       ||
 (/^\.bss_MAC/            && $1 == ".bss_MAC")           ||
 (/^\.boot_resume_stack/  && $1 == ".boot_resume_stack") ||
 (/^\.data/               && $1 == ".data")              ||
 (/^\.bss/                && $1 == ".bss")               ||
 (/^\.heap/               && $1 == ".heap")              ||
 (/^\.stack/              && $1 == ".stack")             ||
 (/^\COMMON/              && $1 == "COMMON")) {
	# Processing map ?
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Section (#%d): '%s' -->\n", FNR, $0
		# Add as new section 
		parseSection()
	}
}

# Parse section
function parseSection() {
	if (bDebug) printf "<!-- parseSection() NF=%d: -->\n", NF
	u32ThisLine = FNR
	asField[1] = $1
	asField[2] = ""
	asField[3] = ""
	# 3 fields ? 
	if (NF >= 3) {
		asField[2] = $2
		asField[3] = $3
	}
	# Only one field ?
	if (NF == 1) {
		# Parse the next (continuation line)
		getline
		if (bDebug) printf "<!------ getline #%d: '%s' -->\n", FNR, $0
		# Note fields from continued line
		asField[2] = $1
		asField[3] = $2
	}
	# Valid data for address and length ?
	if (asField[2] ~ /^0x/ && asField[3] ~ /^0x/) {
		# Note fields from this line
		u32ThisAddr = hexToNum(asField[2])
		u32ThisSize = hexToNum(asField[3])
		if (bDebug) printf "<!------ u32ThisAddr=0x%08x, u32ThisSize=0x%08x -->\n", u32ThisAddr, u32ThisSize
		# Valid size ? 
		if (u32ThisSize > 0) {
			# Got bootloader section to add ?
			if (u32Bootloader > 0) {
				# Is this section in ram ?
				if (u32ThisAddr > au32MemAddr[2]) {
					# Add bootloader section
					addSection("!bootloader", au32MemAddr[2], u32Bootloader, u32ThisLine)
					# Zero bootloader size
					u32Bootloader = 0
				}
			}
			# Add section
			addSection(asField[1], u32ThisAddr, u32ThisSize, u32ThisLine)
		}
	}
}

# Add section
function addSection(sAddName, u32AddAddr, u32AddSize, u32AddLine) {
	if (bDebug) printf "<!-- addSection('%s', 0x%08x, 0x%08x, %d): -->\n", sAddName, u32AddAddr, u32AddSize, u32AddLine
	u32Sections++
	asSection[u32Sections]       = sAddName
	asSectionHtml[u32Sections]   = "HTML-SECTION-" u32Sections
	au32SectionAddr[u32Sections] = u32AddAddr
	au32SectionSize[u32Sections] = u32AddSize
	au32SectionEnd[u32Sections]  = au32SectionAddr[u32Sections] + au32SectionSize[u32Sections]
	au32SectionLine[u32Sections] = u32AddLine
	au32SectionUsed[u32Sections] = 0
	# Loop through banks
	for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
		# Does this section start in this bank ? 
		if (au32SectionAddr[u32Sections] >= au32BankAddr[u32Bank] &&
			au32SectionAddr[u32Sections] <  au32BankAddr[u32Bank] + au32BankSize[u32Bank]) {
			au32SectionBank[u32Sections] = u32Bank
			au32SectionMem[u32Sections]  = au32BankMem[u32Bank]
			if (au32BankSection[u32Bank] == 0) au32BankSection[u32Bank] = u32Sections 
		}
		# Does this section overlap this bank ?
		if (au32SectionAddr[u32Sections] < au32BankEnd[u32Bank] &&
		    au32SectionEnd[u32Sections]  > au32BankAddr[u32Bank]) {
		    # Bank section first not set - set it
		    if (au32BankSectionFirst[u32Bank] == 0) au32BankSectionFirst[u32Bank] = u32Sections
		    # Set bank section last
		    au32BankSectionLast[u32Bank] = u32Sections
		    # Section bank first not set - set it
		    if (au32SectionBankFirst[u32Sections] == 0) au32SectionBankFirst[u32Sections] = u32Bank
		    # Set section bank last
		    au32SectionBankLast[u32Sections] = u32Bank
		}
	}
	# Heap section ?
	if (sAddName == ".heap") {
		# Note heap bank
		u32HeapSection = u32Sections 
	}
	# Discard section ?
    if (sAddName == ".data_discard" ||
        sAddName == ".bss_discard") {
        bDiscard = 1
    }
}

# Symbol " .rodata"
/^ \.rodata/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".rodata" ||
	        asSection[u32Sections] == ".text") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " .after_vectors"
/^ \.after_vectors/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".text") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " .text"
/^ \.text/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".text") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " .data"
/^ \.data/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".data" ||
	        asSection[u32Sections] == ".data_discard") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " .bss"
/^ \.bss/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".bss" ||
	        asSection[u32Sections] == ".bss_discard") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " .scommon"
/^ \.scommon/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".bss") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " COMMON"
/^ COMMON/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".bss" ||
	        asSection[u32Sections] == ".bss_discard") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Symbol " g_u32NwkFrameCounter"
/^ g_u32NwkFrameCounter/ {
	# Processing map ? 
	if (sProcess == "Map") {
		if (bDebug) printf "<!-- Symbol (#%d): '%s' -->\n", FNR, $0
		# In appropriate section ? 
		if (asSection[u32Sections] == ".bss") {
		# ParseSymbol
		parseSymbol()
		}
	}
}

# Parse section
function parseSymbol() {
	if (bDebug) printf "<!-- parseSymbol() NF=%d: -->\n", NF
	u32ThisLine = FNR
	asField[1] = $1
	asField[2] = ""
	asField[3] = ""
	asField[4] = ""
	# 4 fields ? 
	if (NF >= 4) {
		asField[2] = $2
		asField[3] = $3
		asField[4] = $4
	}
	# Only one field ?
	if (NF == 1) {
		# Parse the next (continuation line)
		getline
		if (bDebug) printf "<!------ getline #%d: '%s' -->\n", FNR, $0
		# Note fields from continued line
		asField[2] = $1
		asField[3] = $2
		asField[4] = $3
	}
	# Valid data for address and length ?
	if (asField[2] ~ /^0x/ && asField[3] ~ /^0x/) {
		# Note fields from this line
		u32ThisAddr = hexToNum(asField[2])
		u32ThisSize = hexToNum(asField[3])
		if (bDebug) printf "<!------ u32ThisAddr=0x%08x, u32ThisSize=0x%08x -->\n", u32ThisAddr, u32ThisSize
		# Valid size ? 
		if (u32ThisAddr > 0 && u32ThisSize > 0) {
			# Get filename from path
			sThisFile = pathToFile(asField[4])
			if (bDebug) printf "<!------ sThisFile='%s' -->\n", sThisFile
			# Add symbol
			addSymbol(asField[1], u32ThisAddr, u32ThisSize, u32ThisLine, sThisFile, asField[4])
		}
	}
}

# Extract filename from full path
function pathToFile(sPath, u32PathParts, u32PathPart, asPathSplit, sFile) {
	# Split SubsectionFileFull separators: / \ ( ) 
	u32PathParts = split(sPath, asPathSplit, /\/|\\|\(|\)/)
	# Empty filename
	sFile = ""
	# Get filename from last non-blank folder
	for (u32PathPart = u32PathParts; sFile == "" && u32PathPart > 0; u32PathPart--) {
		sFile = asPathSplit[u32PathPart]
	}
	# Return file
	return sFile
}

# Extract folder from full path
function pathToFolder(sPath, u32PathParts, u32PathPart, asPathSplit, sFolder) {
	# Split SubsectionFileFull separators: / \ ( ) 
	u32PathParts = split(sPath, asPathSplit, /\/|\\|\(|\)/)
	# Last part empty ?
	if (asPathSplit[u32PathParts] == "") {
		# Don't use this part
		u32PathParts--
	}
	# Empty folder
	sFolder = ""
	# Get filename from all parts except last
	for (u32PathPart = 1; u32PathPart < u32PathParts; u32PathPart++) {
		if (asPathSplit[u32PathPart] != ".." && asPathSplit[u32PathPart] != "") {
			if (asPathSplit[u32PathPart] == "gcc") {
				# Start over
				sFolder = ""
			}
			if (sFolder == "") {
				sFolder = asPathSplit[u32PathPart]
			}
			else {
				sFolder = sFolder "/" asPathSplit[u32PathPart]
			}
		}
	}
	# Return folder
	return sFolder
}

# Add symbol
function addSymbol(sAddName, u32AddAddr, u32AddSize, u32AddLine, sAddFile, sAddPath) {
	if (bDebug) printf "<!-- addSymbol('%s', 0x%08x, 0x%08x, %d, '%s', '%s'): -->\n", sAddName, u32AddAddr, u32AddSize, u32AddLine, sAddFile, sAddPath
	if (sAddName == ".text.PWRM_vManagePower") bPwrmManagePower = 1
	if (sAddName == ".text.PWR_EnterLowPower") bPwrmManagePower = 1
	# If this a new file ? 
	if (sAddFile != asFile[u32Files]) {
		# Add as new file
		addFile(sAddFile, sAddPath, u32AddAddr, u32AddSize, u32AddLine)
	}
	# Add new symbol
	u32Symbols++
	asSymbol[u32Symbols]          = sAddName
	asSymbolHtml[u32Symbols]      = "HTML-SYMBOL-" u32Symbols
	au32SymbolAddr[u32Symbols]    = u32AddAddr
	au32SymbolSize[u32Symbols]    = u32AddSize
	au32SymbolEnd[u32Symbols]     = au32SymbolAddr[u32Symbols] + au32SymbolSize[u32Symbols]
	au32SymbolLine[u32Symbols]    = u32AddLine
	au32SymbolUsed[u32Symbols]    = 0
	au32SymbolFile[u32Symbols]    = u32Files
	au32SymbolSection[u32Symbols] = u32Sections
	au32SymbolBank[u32Symbols]    = au32SectionBank[u32Sections]
	au32SymbolMem[u32Symbols]     = au32SectionMem[u32Sections]
	if (au32FileSymbol[u32Files] == 0) au32FileSymbol[u32Files] = u32Symbols
	# Update file
	updateFile(au32SymbolEnd[u32Symbols])
}

# Add file
function addFile(sAddName, sAddPath, u32AddAddr, u32AddSize, u32AddLine) {
	if (bDebug) printf "<!-- addFile('%s', 0x%08x, 0x%08x, %d): -->\n", sAddName, u32AddAddr, u32AddSize, u32AddLine
	u32Files++
	asFile[u32Files]          = sAddName
	asFilePath[u32Files]      = sAddPath
	asFileHtml[u32Files]      = "HTML-FILE-" u32Files
	au32FileAddr[u32Files]    = u32AddAddr
	au32FileSize[u32Files]    = u32AddSize
	au32FileEnd[u32Files]     = au32SectionAddr[u32Sections] + au32SectionSize[u32Sections]
	au32FileLine[u32Files]    = u32AddLine
	au32FileUsed[u32Files]    = 0
	au32FileSection[u32Files] = u32Sections
	au32FileBank[u32Files]    = au32SectionBank[u32Sections]
	au32FileMem[u32Files]     = au32SectionMem[u32Sections]
	if (au32SectionFile[u32Sections] == 0) au32SectionFile[u32Sections] = u32Files
	au32SectionFiles[u32Sections]++
}

# Update file
function updateFile(u32UpdateEnd) {
	if (bDebug) printf "<!-- updateFile(0x%08x): -->\n", u32UpdateEnd
	au32FileEnd[u32Files]  = u32UpdateEnd
	au32FileSize[u32Files] = au32FileEnd[u32Files] - au32FileAddr[u32Files]
}

# Record begins "OUTPUT(" - end of memory map
/^OUTPUT\(/ {
	# Stop processing data from here
	sProcess = "End"
}

# Post-processing END
END {
	# RAM0 memory present ? 
	if (asMem[2] == "RAM0") {
		addSection("!warmstart", hexToNum("0x4015FE0"), hexToNum("0x20"), u32ThisLine)
	}
	# Fill any gaps
	fillGaps()
	# Calculate used memory
	calculateUsed()
	# Calculate retained
	calculateRetain()
	# Calculate folders
	if (u32SourceFiles != 0) calculateFolders()
	# Get filename
	sFileName = pathToFile(FILENAME)
	# Get path without extension
	sPathNoExt = substr(FILENAME, 1, index(FILENAME, ".map"))
	# Build output file paths
	sPathTxt  = sPathNoExt "txt"
	sPathXml  = sPathNoExt "xml"
	sPathHtml = sPathNoExt "html"
	# Set colors
	sColorN    = "#FFB500"
	sColorNx   = "#958437"
	sColorX    = "#7BB1DB"
	sColorXp   = "#739833"
	sColorP    = "#C9D200"
	sDarkN     = "#7F5900"
	sDarkX     = "#3D576B"
	sDarkP     = "#656800"
	sLightN    = "#FFD87F"
	sLightX    = "#A9C4D8"
	sLightP    = "#CDD168"

	# TEXT OUTPUT =================================================================================
	# Output text ? 
	if (bOutputText) {
		printf "%s\n", sFileName > sPathTxt
		printf "MEMORIES -----------------------------------------------------------------------\n" > sPathTxt
		for (u32Mem = 1; u32Mem <= u32Mems; u32Mem++) {
			printf "%6d MEM: %s (%dkb) \n", 
			au32MemSize[u32Mem],
			asMem[u32Mem],
			au32MemSize[u32Mem]/1024 > sPathTxt
		}
		printf "BANKS --------------------------------------------------------------------------\n" > sPathTxt
		for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
			printf "%6d BANK: %s | %s \n", 
			au32BankSize[u32Bank],
			asMem[au32BankMem[u32Bank]],
			asBank[u32Bank] > sPathTxt
		}
		printf "SECTIONS -----------------------------------------------------------------------\n" > sPathTxt
		for (u32Section = 1; u32Section <= u32Sections; u32Section++) {
			printf "%6d SECTION: %s | %s | %s \n", 
			au32SectionSize[u32Section],
			asMem[au32SectionMem[u32Section]],
			asBank[au32SectionBank[u32Section]],
			asSection[u32Section] > sPathTxt
		}
		printf "FILES --------------------------------------------------------------------------\n" > sPathTxt
		for (u32File = 1; u32File <= u32Files; u32File++) {
			printf "%6d FILE: %s | %s | %s | %s \n", 
			au32FileSize[u32File],
			asMem[au32FileMem[u32File]],
			asBank[au32FileBank[u32File]],
			asSection[au32FileSection[u32File]],
			asFile[u32File] > sPathTxt
		}
		printf "SYMBOLS ------------------------------------------------------------------------\n" > sPathTxt
		for (u32Symbol = 1; u32Symbol <= u32Symbols; u32Symbol++) {
			printf "%6d SYMBOL: %s | %s | %s | %s | %s \n", 
			au32SymbolSize[u32Symbol],
			asMem[au32SymbolMem[u32Symbol]],
			asBank[au32SymbolBank[u32Symbol]],
			asSection[au32SymbolSection[u32Symbol]],
			asFile[au32SymbolFile[u32Symbol]],
			asSymbol[u32Symbol] > sPathTxt
		}
		if (u32SourceFiles != 0) {
			printf "FOLDERS --------------------------------------------------------------------\n" > sPathTxt
			for (u32Folder = 1; u32Folder <= u32Folders; u32Folder++) {
				printf "%6d FOLDER: %s | %s | %s \n", 
				au32FolderSize[u32Folder],
				asMem[au32FolderMem[u32Folder]],
				asSection[au32FolderSection[u32Folder]],
				asFolder[u32Folder] > sPathTxt
			}
		}
		if (u32SourceFiles != 0) {
			printf "TREE -----------------------------------------------------------------------\n" > sPathTxt
			for (u32Tree = 1; u32Tree <= u32Trees; u32Tree++) {
				printf "%6d TREE: %s | %s | %s \n", 
				au32TreeSize[u32Tree],
				asMem[au32TreeMem[u32Tree]],
				asSection[au32TreeSection[u32Tree]],
				asTree[u32Tree] > sPathTxt
			}
		}
	}

	# XML OUTPUT =================================================================================
	# Output xml ? 
	if (bOutputXml) {
		sXmlHeader         = "<?xml version=\"1.0\"?>\n<?mso-application progid=\"Excel.Sheet\"?>\n"
		sXmlWorkbookStart  = "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n" \
		                     "          xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n"
		sXmlWorkbookEnd    = "</Workbook>\n"   
		sXmlStyles         = "  <ss:Styles>\n" \
		                     "    <ss:Style ss:ID=\"Left\"><Alignment ss:Horizontal=\"Left\"/></ss:Style>\n" \
		                     "    <ss:Style ss:ID=\"Right\"><Alignment ss:Horizontal=\"Right\"/></ss:Style>\n" \
		                     "    <ss:Style ss:ID=\"LeftBold\"><Alignment ss:Horizontal=\"Left\"/><ss:Font ss:Bold=\"1\"/></ss:Style>\n" \
		                     "    <ss:Style ss:ID=\"RightBold\"><Alignment ss:Horizontal=\"Right\"/><ss:Font ss:Bold=\"1\"/></ss:Style>\n" \
                             "  </ss:Styles>\n"
		sXmlWorksheetStart = "  <Worksheet ss:Name=\"%s\">\n"
		sXmlWorksheetEnd   = "  </Worksheet>"                            
		sXmlTableStart     = "    <Table>\n"
		sXmlColumn         = "      <Column ss:Width=\"%d\"/>\n"
		sXmlTableEnd       = "    </Table>\n"
		sXmlRowStart       = "      <Row>\n"
		sXmlRowEnd         = "      </Row>\n"
		sXmlCellLeft       = "        <Cell ss:StyleID=\"Left\"><Data ss:Type=\"String\">%s</Data></Cell>\n"
		sXmlCellRight      = "        <Cell ss:StyleID=\"Right\"><Data ss:Type=\"String\">%s</Data></Cell>\n"
		sXmlCellLeftBold   = "        <Cell ss:StyleID=\"LeftBold\"><Data ss:Type=\"String\">%s</Data></Cell>\n"
		sXmlCellRightBold  = "        <Cell ss:StyleID=\"RightBold\"><Data ss:Type=\"String\">%s</Data></Cell>\n"
		sXmlCellNumber     = "        <Cell><Data ss:Type=\"Number\">%d</Data></Cell>\n"
		u32XmlWidthSize    = 50
		u32XmlWidthMemory  = 50
		u32XmlWidthBank    = 90

		printf sXmlHeader                     > sPathXml
	    printf sXmlWorkbookStart              > sPathXml
	    printf sXmlStyles                     > sPathXml
	    
	    # MEMORIES --------------------------------------------------------------------------------
		printf sXmlWorksheetStart, "Memories" > sPathXml
		printf sXmlTableStart                 > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlColumn, u32XmlWidthMemory  > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlRowStart                   > sPathXml
		printf sXmlCellRightBold, "Size (b)"  > sPathXml
		printf sXmlCellRightBold, "Used (b)"  > sPathXml
		printf sXmlCellRightBold, "Used (%)" > sPathXml
		printf sXmlCellLeftBold, "Memory"     > sPathXml
		printf sXmlCellRightBold, "Size (kb)" > sPathXml
		printf sXmlRowEnd                     > sPathXml
		for (u32Mem = 1; u32Mem <= u32Mems; u32Mem++) {
			u32ThisPc = (au32MemUsed[u32Mem] * 100) / au32MemSize[u32Mem]
		    if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			printf sXmlRowStart                               > sPathXml
			printf sXmlCellNumber, au32MemSize[u32Mem]        > sPathXml
			printf sXmlCellNumber, au32MemUsed[u32Mem]        > sPathXml
			printf sXmlCellNumber,     u32ThisPc                  > sPathXml
			printf sXmlCellLeft,   asMem[u32Mem]              > sPathXml
			printf sXmlCellNumber, (au32MemSize[u32Mem]/1024) > sPathXml
			printf sXmlRowEnd                                 > sPathXml
		}
		printf sXmlTableEnd     > sPathXml
		printf sXmlWorksheetEnd > sPathXml
		# BANKS -----------------------------------------------------------------------------------
		printf sXmlWorksheetStart, "Banks"    > sPathXml
		printf sXmlTableStart                 > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlColumn, u32XmlWidthSize    > sPathXml
		printf sXmlColumn, u32XmlWidthMemory  > sPathXml
		printf sXmlColumn, u32XmlWidthBank    > sPathXml
		printf sXmlRowStart                   > sPathXml
		printf sXmlCellRightBold, "Size (b)"  > sPathXml
		printf sXmlCellRightBold, "Used (b)"  > sPathXml
		printf sXmlCellRightBold, "Used (%)"  > sPathXml
		printf sXmlCellLeftBold, "Memory"     > sPathXml
		printf sXmlCellLeftBold, "Bank"       > sPathXml
		printf sXmlRowEnd                     > sPathXml
		for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
			u32ThisPc = (au32BankUsed[u32Bank] * 100) / au32BankSize[u32Bank]
		    if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			printf sXmlRowStart                              > sPathXml
			printf sXmlCellNumber, au32BankSize[u32Bank]     > sPathXml
			printf sXmlCellNumber, au32BankUsed[u32Bank]     > sPathXml
			printf sXmlCellNumber,     u32ThisPc                 > sPathXml
			printf sXmlCellLeft, asMem[au32BankMem[u32Bank]] > sPathXml
			printf sXmlCellLeft, asBank[u32Bank]             > sPathXml
			printf sXmlRowEnd                                > sPathXml
		}
		printf sXmlTableEnd     > sPathXml
		printf sXmlWorksheetEnd > sPathXml
	    printf sXmlWorkbookEnd  > sPathXml
if (0) {		
		printf "SECTIONS -----------------------------------------------------------------------\n" > sPathTxt
		for (u32Section = 1; u32Section <= u32Sections; u32Section++) {
			printf "%6d SECTION: %s | %s | %s \n", 
			au32SectionSize[u32Section],
			asMem[au32SectionMem[u32Section]],
			asBank[au32SectionBank[u32Section]],
			asSection[u32Section] > sPathTxt
		}
		printf "FILES --------------------------------------------------------------------------\n" > sPathTxt
		for (u32File = 1; u32File <= u32Files; u32File++) {
			printf "%6d FILE: %s | %s | %s | %s \n", 
			au32FileSize[u32File],
			asMem[au32FileMem[u32File]],
			asBank[au32FileBank[u32File]],
			asSection[au32FileSection[u32File]],
			asFile[u32File] > sPathTxt
		}
		printf "SYMBOLS ------------------------------------------------------------------------\n" > sPathTxt
		for (u32Symbol = 1; u32Symbol <= u32Symbols; u32Symbol++) {
			printf "%6d SYMBOL: %s | %s | %s | %s | %s \n", 
			au32SymbolSize[u32Symbol],
			asMem[au32SymbolMem[u32Symbol]],
			asBank[au32SymbolBank[u32Symbol]],
			asSection[au32SymbolSection[u32Symbol]],
			asFile[au32SymbolFile[u32Symbol]],
			asSymbol[u32Symbol] > sPathTxt
		}
		if (u32SourceFiles != 0) {
			printf "FOLDERS --------------------------------------------------------------------\n" > sPathTxt
			for (u32Folder = 1; u32Folder <= u32Folders; u32Folder++) {
				printf "%6d FOLDER: %s | %s | %s \n", 
				au32FolderSize[u32Folder],
				asMem[au32FolderMem[u32Folder]],
				asSection[au32FolderSection[u32Folder]],
				asFolder[u32Folder] > sPathTxt
			}
		}
		if (u32SourceFiles != 0) {
			printf "TREE -----------------------------------------------------------------------\n" > sPathTxt
			for (u32Tree = 1; u32Tree <= u32Trees; u32Tree++) {
				printf "%6d TREE: %s | %s | %s \n", 
				au32TreeSize[u32Tree],
				asMem[au32TreeMem[u32Tree]],
				asSection[au32TreeSection[u32Tree]],
				asTree[u32Tree] > sPathTxt
			}
		}
}		
	}

	# HTML OUTPUT =================================================================================
	# Output html ? 
	if (bOutputHtml) {
		printf "<!DOCTYPE html>\n" > sPathHtml
		printf "<!-- %s -->\n", sFileName > sPathHtml
	}
	# Output html ? 
	if (bOutputHtml) {
		printf "<!-- HTML =====================================================================-->\n" > sPathHtml 
		printf "<html>\n" > sPathHtml
		printf "<!-- HTML - HEAD ---------------------------------------------------------------->\n" > sPathHtml
		printf "<head>\n" > sPathHtml
		printf "<meta charset=\"UTF-8\">\n" > sPathHtml
		printf "<title>%s</title>\n", sFileName > sPathHtml
		printf "<!-- CSS - STYLES --------------------------------------------------------------->\n" > sPathHtml
		printf "<style>\n" > sPathHtml
		printf "body { font-family: Arial, Helvetica, sans-serif; }\n" > sPathHtml
		printf ".out_l, .out_l, .out_l { text-align: left;  vertical-align: top; border: 0px solid red; top; border-collapse: collapse; font-family: Arial, Helvetica, sans-serif; }\n" > sPathHtml
		printf ".out_r                 { text-align: right; vertical-align: top; border: 0px solid red; top; border-collapse: collapse; font-family: Arial, Helvetica, sans-serif; }\n" > sPathHtml
		printf "table, tr, th, td { text-align: right; vertical-align: top; border: 1px solid black; border-collapse: collapse; font-family: \"Courier New\", monospace; }\n" > sPathHtml
#		printf "th, td { padding-left: 2px; padding-right: 2px; }\n" > sPathHtml
		printf "small {color: #800000; }\n" > sPathHtml
		printf ".bg_nx { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", sColorNx > sPathHtml
		printf ".bg_xp { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", sColorXp > sPathHtml
		printf ".bg_n  { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", sColorN > sPathHtml
		printf ".bg_x  { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", sColorX > sPathHtml
		printf ".bg_p  { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", sColorP > sPathHtml
		printf ".bg_ln { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", "#FFFFFF" > sPathHtml
		printf ".bg_lx { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", "#FFFFFF" > sPathHtml
		printf ".bg_lp { background-color: %s; text-align: left; font-family: Arial, Helvetica, sans-serif; }\n", "#FFFFFF" > sPathHtml
		printf ".left { text-align: left; font-family: Arial, Helvetica, sans-serif; }\n" > sPathHtml
		printf ".left_grey { text-align: left; font-family: Arial, Helvetica, sans-serif; color: #808080; }\n" > sPathHtml
		printf ".right { text-align: right; font-family: Arial, Helvetica, sans-serif; }\n" > sPathHtml
		printf ".index { font-size: small; }\n" > sPathHtml
		printf ".error { background-color: #FF8080; }\n" > sPathHtml
		printf ".warning { background-color: #FFFF80; }\n" > sPathHtml
#		printf ".columnleft { float: left; }\n" > sPathHtml
#		printf ".columnright { float: right; width: 270px; }\n" > sPathHtml
#		printf ".row:after { content: \"\"; display: table; clear: both; }\n" > sPathHtml
#		printf ".map { width: %dpx; }\n", u32ImageWidth  > sPathHtml
#		printf "div {padding-left: 2; padding-right: 2 }\n" > sPathHtml
		printf "</style>\n" > sPathHtml
		printf "</head>\n" > sPathHtml
		printf "<!-- HTML - BODY ---------------------------------------------------------------->\n" > sPathHtml
		printf "<body>\n" > sPathHtml
		printf "<table class=\"out_l\">\n" > sPathHtml
		printf "<tr class=\"out_l\">\n" > sPathHtml
		printf "<td class=\"out_l\" colspan=2>\n" > sPathHtml
		# Main heading and intro
		printf "<h1 id=\"top\">%s\n", sFileName > sPathHtml
		if (bOutputIds) printf " <small>(%s)</small>", sFileName > sPathHtml
		printf "</h1>\n", sFileName > sPathHtml
		printf "<p>This file contains an analysis of the <b>%s</b> file.</p>\n", sFileName > sPathHtml
		printf "<p>This HTML view is useful for exploring the statically allocated data in a\n" > sPathHtml
		printf "particular build. To compare two different builds or versions these HTML files can\n"  > sPathHtml
		printf "be loaded into a diff tool. HTML comments at the top of the HTML code list just the\n" > sPathHtml
		printf "sizes of the banks, sections, files and symbols making identification of differences\n" > sPathHtml
		printf "easy.</p>\n" > sPathHtml
		printf "<p>Gaps of 16 bytes or less are assumed to be fills and added to the previous\n" > sPathHtml
		printf "item's size. Fills are not explicitly handled.</p>\n" > sPathHtml
		printf "<p>Percentages are rounded up to the nearest whole percent.</p>\n" > sPathHtml
		printf "<p>In the tables below the Line column specifies the line number in the input map file" > sPathHtml
		printf " that row was derived from.</p>\n" > sPathHtml
		printf "<p>Some cells are colour coded as follows:</p>\n" > sPathHtml
		printf "<dl>\n" > sPathHtml
		printf "<dt><b>Yellow</b></dt>\n" > sPathHtml
		printf "<dd>Indicates an unexpected start or end address. These are usually caused by the compiler overlaying indentical data" > sPathHtml
		printf " to reduce memory usage and are intentional. This is most often seen in read-only (rodata) symbols which are often strings" > sPathHtml
		printf " stored in flash memory. As this file lists all allocated symbols calculated totals may exceed the expected value.</dd>\n" > sPathHtml
		printf "<dt><b>Red</b></dt>\n" > sPathHtml
		printf "<dd>Indicates an unexpected total size or percentage. These are usually knock-on effects from the compiler overlaying" > sPathHtml
		printf " identical data and do not necessarily indicate a problem.</dd>\n" > sPathHtml
		printf "</dl>\n" > sPathHtml
		# Output index
		htmlIndex()
		printf "<!-- MEMORIES - TABLE ----------------------------------------------------------->\n" > sPathHtml
		# Output memories
		htmlMemories()
		# Flash heading and intro
		# RAM heading and intro
		printf "</td>\n"  > sPathHtml
		printf "</tr>\n"  > sPathHtml
		printf "<!-- HTML - RAM --------------------------------------------------------------->\n" > sPathHtml
		# Output memory info
		htmlMem(2)
		# Set up right div
		# Output memory info
		htmlMem(1)
	}
	# Output html ? 
	if (bOutputHtml) {
		printf "<!-- FOOTER --------------------------------------------------------------------->\n" > sPathHtml
		printf "</table>\n" > sPathHtml
		printf "</body>\n" > sPathHtml
		printf "</html>\n" > sPathHtml
	}
	exit
}

# Convert hex string to numeric
function hexToNum(sHex, u32Num, u32Len, u32Pos, sChar, u32Idx, c)
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

# Fill small gaps
function fillGaps() {
	# Loop through sections (except last)
	for (u32Section = 1; u32Section < u32Sections; u32Section++) {
		# Is there a gap ?
		if (au32SectionEnd[u32Section] != au32SectionAddr[u32Section+1]) {
			# Calculate gap
			u32Gap = au32SectionAddr[u32Section+1] - au32SectionEnd[u32Section]
			# Only a small gap ?
			if (u32Gap > 0 && u32Gap < 16) {
				# Fill it
				au32SectionSize[u32Section] += u32Gap
				au32SectionEnd[u32Section]  = au32SectionAddr[u32Section] + au32SectionSize[u32Section] 
			}
			# Big gap ?
			else {
				au32SectionGap[u32Section] = u32Gap
			}
		}				
	}
	# Loop through files (except last)
	for (u32File = 1; u32File <= u32Files; u32File++) {
		# Not last file and there a gap ?
		if (u32File != u32Files && au32FileEnd[u32File] != au32FileAddr[u32File+1]) {
			# Calculate gap
			u32Gap = au32FileAddr[u32File+1] - au32FileEnd[u32File]
			# Only a small gap ?
			if (u32Gap > 0 && u32Gap < 16) {
				# Fill it
				au32FileSize[u32File] += u32Gap
				au32FileEnd[u32File]  = au32FileAddr[u32File] + au32FileSize[u32File] 
			}
			# Big gap ?
			else {
				au32FileGap[u32File] = u32Gap
			}
		}		
		# Last file or last file in section ?
		if (u32File == u32Files || au32FileSection[u32File] != au32FileSection[u32File+1]) {
			# Not aligned to end of section ?
			if (au32FileEnd[u32File] != au32SectionEnd[au32FileSection[u32File]]) {
				# Align to end of section
				au32FileEnd[u32File] = au32SectionEnd[au32FileSection[u32File]]
				au32FileSize[u32File] = au32FileEnd[u32File] - au32FileAddr[u32File]
			}
		}
	}
	# Loop through symbols (except last)
	for (u32Symbol = 1; u32Symbol <= u32Symbols; u32Symbol++) {
		# Not last symbol and there a gap ?
		if (u32Symbol != u32Symbols && au32SymbolEnd[u32Symbol] != au32SymbolAddr[u32Symbol+1]) {
			# Calculate gap
			u32Gap = au32SymbolAddr[u32Symbol+1] - au32SymbolEnd[u32Symbol]
			# Only a small gap ?
			if (u32Gap > 0 && u32Gap < 16) {
				# Fill it
				au32SymbolSize[u32Symbol] += u32Gap
				au32SymbolEnd[u32Symbol]  = au32SymbolAddr[u32Symbol] + au32SymbolSize[u32Symbol] 
			}
			# Big gap ?
			else {
				au32SymbolGap[u32Symbol] = u32Gap
			}
		}				
		# Last symbol or last symbol in file ?
		if (u32Symbol == u32Symbols || au32SymbolFile[u32Symbol] != au32SymbolFile[u32Symbol+1]) {
			# Not aligned to end of file ?
			if (au32SymbolEnd[u32File] < au32FileEnd[au32SymbolFile[u32Symbol]]) {
				# Align to end of file
				au32SymbolEnd[u32Symbol] = au32FileEnd[au32SymbolFile[u32Symbol]]
				au32SymbolSize[u32Symbol] = au32SymbolEnd[u32Symbol] - au32SymbolAddr[u32Symbol]
			}
		}
	}
}

# Claculate used
function calculateUsed() {
	# Loop through symbols
	for (u32Symbol = 1; u32Symbol <= u32Symbols; u32Symbol++) {
		# Update used in symbol's file
		au32FileUsed[au32SymbolFile[u32Symbol]] += au32SymbolSize[u32Symbol]
	}
	# Loop through files
	for (u32File = 1; u32File <= u32Files; u32File++) {
		# Update used in file's section
		au32SectionUsed[au32FileSection[u32File]] += au32FileSize[u32File]
	}
	# Loop through sections
	for (u32Section = 1; u32Section <= u32Sections; u32Section++) {
		# No files in section ?
		if (au32SectionFiles[u32Section] == 0) au32SectionUsed[u32Section] = au32SectionSize[u32Section]
		# Loop through banks
		for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
			# Does this section use any of this bank ? 
			if (au32SectionAddr[u32Section] <= au32BankEnd[u32Bank] &&
				au32SectionEnd[u32Section]  >= au32BankAddr[u32Bank]) {
				# Calculate start of used part
				if (au32SectionAddr[u32Section] < au32BankAddr[u32Bank]) u32UsedAddr = au32BankAddr[u32Bank]
				else                                                     u32UsedAddr = au32SectionAddr[u32Section]
				# Calculate end of used part
				if (au32SectionEnd[u32Section] > au32BankEnd[u32Bank])   u32UsedEnd = au32BankEnd[u32Bank]
				else                                                     u32UsedEnd = au32SectionEnd[u32Section]
				# Used data in this bank ?
				if (u32UsedEnd > u32UsedAddr) {
					# Calculate used size
					u32UsedSize = u32UsedEnd - u32UsedAddr
					# Update used data in bank
					au32BankUsed[u32Bank] += u32UsedSize
				}
			}
		}
	}
	# Loop through banks
	for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
		# Update used in file's section
		au32MemUsed[au32BankMem[u32Bank]] += au32BankUsed[u32Bank]
	}
}

# Calculate retained
function calculateRetain() {
	# Power manager used - check RAM for retained data 
	if (bPwrmManagePower != 0) {
		# Clear found flag
		bFound = 0
		# Loop through sections 
		for (u32Section = 1; u32Section <= u32Sections && bFound == 0; u32Section++) {
			# Is this the heap section ?
			if (asSection[u32Section] == ".heap") {
				# Loop through banks from the heap's to the last
				for (u32Bank = au32SectionBank[u32Section]; u32Bank <= u32Banks; u32Bank++) {
					# In RAM memory ? 
					if (au32BankMem[u32Bank] == 2) {
						au32BankRetain[u32Bank] += au32BankSize[u32Bank]
						au32MemRetain[au32BankMem[u32Bank]] += au32BankSize[u32Bank]
					}
				}
				# Note we found retained data 
				bFound = 1
			}
			# Is this the data section ?
			if (asSection[u32Section] == ".data") {
				# Loop through banks from the heap's to the first
				for (u32Bank = au32SectionBank[u32Section]; u32Bank >= 1; u32Bank--) {
					# In RAM memory ? 
					if (au32BankMem[u32Bank] == 2) {
						au32BankRetain[u32Bank] += au32BankSize[u32Bank]
						au32MemRetain[au32BankMem[u32Bank]] += au32BankSize[u32Bank]
					}
				}
				# Note we found retained data 
				bFound = 1
			}
		}
	}
	# Loop through banks
	for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
		# In FLASH memory ? 
		if (au32BankMem[u32Bank] == 1) {
			au32BankRetain[u32Bank] += au32BankSize[u32Bank]
			au32MemRetain[au32BankMem[u32Bank]] += au32BankSize[u32Bank]
		}
	}
}

# Calculate folders
function calculateFolders(sSource, sFolder, sType, u32Sort1, u32Sort2) {
	# Loop through files
	for (u32File = 1; u32File <= u32Files; u32File++) {
		sSource = "";
		sType = "";
		# Loop through source files
		for (u32SourceFile = 1; u32SourceFile <= u32SourceFiles && sSource == ""; u32SourceFile++) {
			# Does this file match the output source file ? */
			if (asFilePath[u32File] == asSourceFileOutput[u32SourceFile]) {
				# Found a match - use input file as source
				sSource = asSourceFileInput[u32SourceFile]
				sType = asSourceFileType[u32SourceFile]
			}
		}
		# Not matched ? */
		if (sSource == "") {
			# Use original path
			sSource = asFilePath[u32File]
		}
		if (sType == "") {
			# Assume library
			sType = "LIB"
		}
		# Get folder name from source
		sFolder = sType "/" pathToFolder(sSource)
		# Update folder size 
		updateFolder(sFolder, au32FileSection[u32File], au32FileMem[u32File], au32FileSize[u32File])
		# Add filename to folder name for tree
		sFolder = sFolder "/" asFile[u32File]
		# Update tree size 
		updateTree(sFolder, au32FileSection[u32File], au32FileMem[u32File], au32FileSize[u32File])
	}
	# Sort folders
	u32Sort1 = 1;
	# Loop through the folders
	for (u32Sort2 = u32Sort1+1; u32Sort2 <= u32Folders; u32Sort2++) {
		# Memory/section is different ?
		if (au32FolderSection[u32Sort1] != au32FolderSection[u32Sort2] ||
		    au32FolderMem[u32Sort1]     != au32FolderMem[u32Sort2]) {
			# Sort this set of folders
			qsortFolder(u32Sort1, u32Sort2-1)
			# Get ready for next set
			u32Sort1 = u32Sort2;
		}
		# Last folder ?
		else if (u32Sort2 == u32Folders) {
			# Sort final set of folders
			qsortFolder(u32Sort1, u32Sort2)
		}
	}
	# Loop through folders assigning html ids
	for (u32Sort1 = 1; u32Sort1 <= u32Folders; u32Sort1++) {
		asFolderHtml[u32Sort1] = "HTML-FOLDER-" u32Sort1
	}
	# Sort trees
	u32Sort1 = 1;
	# Loop through the trees
	for (u32Sort2 = u32Sort1+1; u32Sort2 <= u32Trees; u32Sort2++) {
		# Memory/section is different ?
		if (au32TreeSection[u32Sort1] != au32TreeSection[u32Sort2] ||
		    au32TreeMem[u32Sort1]     != au32TreeMem[u32Sort2]) {
			# Sort this set of trees
			qsortTree(u32Sort1, u32Sort2-1)
			# Get ready for next set
			u32Sort1 = u32Sort2;
		}
		# Last folder ?
		else if (u32Sort2 == u32Folders) {
			# Sort final set of trees
			qsortTree(u32Sort1, u32Sort2)
		}
	}
	# Loop through trees assigning html ids
	for (u32Sort1 = 1; u32Sort1 <= u32Trees; u32Sort1++) {
		asTreeHtml[u32Sort1] = "HTML-TREE-" u32Sort1
	}
}

# Update (add) folder size 
function updateFolder(sAddFolder, u32AddSection, u32AddMem, u32AddSize, u32Folder, bFound) {
	# Not found
	bFound = 0
	# Loop through folders
	for (u32Folder = 1; u32Folder <= u32Folders && bFound == 0; u32Folder++) {
		# Need to update this folder ? 
		if (asFolder[u32Folder]          == sAddFolder    &&
		    au32FolderSection[u32Folder] == u32AddSection &&
		    au32FolderMem[u32Folder]     == u32AddMem) {
			# We have found the folder
			bFound = 1
			# Update the size
			au32FolderSize[u32Folder] += u32AddSize
			au32FolderFiles[u32Folder]++
		}
	}
	# Need to add new folder ? 
	if (bFound == 0) {
		u32Folders++
		asFolder[u32Folders]          = sAddFolder
		au32FolderSection[u32Folders] = u32AddSection
		au32FolderMem[u32Folders]     = u32AddMem
		au32FolderSize[u32Folders]    = u32AddSize
		au32FolderFiles[u32Folders]   = 1
	}
}

# Quick sort folders
function qsortFolder(u32Left, u32Right, u32I, u32Last) {
	if (u32Left >= u32Right) {
		return
	}
	qsortFolderSwap(u32Left, u32Left+int((u32Right-u32Left+1)*rand()))
	u32Last = u32Left
	for (u32I = u32Left+1; u32I <= u32Right; u32I++) {
		if (asFolder[u32I] < asFolder[u32Left]) {
			qsortFolderSwap(++u32Last, u32I)
		}
	}
	qsortFolderSwap(u32Left, u32Last)
	qsortFolder(u32Left, u32Last-1)
	qsortFolder(u32Last+1, u32Right)
}

# Quick sort folders swap
function qsortFolderSwap(u32I, u32J, sTemp, u32Temp) {
	sTemp = asFolder[u32I]
	asFolder[u32I] = asFolder[u32J]
	asFolder[u32J] = sTemp
	
	u32Temp = au32FolderSection[u32I]
	au32FolderSection[u32I] = au32FolderSection[u32J]
	au32FolderSection[u32J] = u32Temp
	
	u32Temp = au32FolderMem[u32I]
	au32FolderMem[u32I] = au32FolderMem[u32J]
	au32FolderMem[u32J] = u32Temp
	
	u32Temp = au32FolderSize[u32I]
	au32FolderSize[u32I] = au32FolderSize[u32J]
	au32FolderSize[u32J] = u32Temp

	u32Temp = au32FolderFiles[u32I]
	au32FolderFiles[u32I] = au32FolderFiles[u32J]
	au32FolderFiles[u32J] = u32Temp
}

# Update (add) tree size 
function updateTree(sAddFolder, u32AddSection, u32AddMem, u32AddSize, u32Tree, bFound, u32TreeParts, u32TreePart, asTreeSplit, sTree, sNbsp) {
	# Split SubsectionFileFull separators: / \ ( ) 
	u32TreeParts = split(sAddFolder, asTreeSplit, /\/|\\|\(|\)/)
	# Empty tree
	sTree = ""
	sNbsp = ""
	# Loop through tree parts
	for (u32TreePart = 1; u32TreePart <= u32TreeParts; u32TreePart++) {
		# Set leaf
		sTree = sTree asTreeSplit[u32TreePart]
		if (u32TreePart != u32TreeParts) sTree = sTree " / "
		if (u32TreePart != 1) sNbsp = sNbsp "&verbar;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
		# Not found
		bFound = 0
		# Loop through tree
		for (u32Tree = 1; u32Tree <= u32Trees && bFound == 0; u32Tree++) {
			# Need to update this tree ? 
			if (asTree[u32Tree]          == sTree    &&
				au32TreeSection[u32Tree] == u32AddSection &&
				au32TreeMem[u32Tree]     == u32AddMem) {
				# We have found the tree
				bFound = 1
				# Update the size
				au32TreeSize[u32Tree] += u32AddSize
			}
		}
		# Need to add new tree ? 
		if (bFound == 0) {
			u32Trees++
			asTree[u32Trees]          = sTree
			if (u32TreePart != u32TreeParts) asTreeDisplay[u32Trees] = asTreeDisplay[u32Trees] "/"
			au32TreeSection[u32Trees] = u32AddSection
			au32TreeMem[u32Trees]     = u32AddMem
			au32TreeSize[u32Trees]    = u32AddSize
			if (u32TreePart == u32TreeParts) {			
				au32TreeLeaf[u32Trees]  = 1
				asTreeDisplay[u32Trees] = sNbsp asTreeSplit[u32TreePart] 
			}
			else {
				au32TreeLeaf[u32Trees] = 0
				asTreeDisplay[u32Trees] = sNbsp "<b>" asTreeSplit[u32TreePart] "/</b>"
			}
		}
	}
}

# Quick sort trees
function qsortTree(u32Left, u32Right, u32I, u32Last) {
	if (u32Left >= u32Right) {
		return
	}
	qsortTreeSwap(u32Left, u32Left+int((u32Right-u32Left+1)*rand()))
	u32Last = u32Left
	for (u32I = u32Left+1; u32I <= u32Right; u32I++) {
		if (asTree[u32I] < asTree[u32Left]) {
			qsortTreeSwap(++u32Last, u32I)
		}
	}
	qsortTreeSwap(u32Left, u32Last)
	qsortTree(u32Left, u32Last-1)
	qsortTree(u32Last+1, u32Right)
}

# Quick sort tree swap
function qsortTreeSwap(u32I, u32J, sTemp, u32Temp) {
	sTemp = asTree[u32I]
	asTree[u32I] = asTree[u32J]
	asTree[u32J] = sTemp
	
	sTemp = asTreeDisplay[u32I]
	asTreeDisplay[u32I] = asTreeDisplay[u32J]
	asTreeDisplay[u32J] = sTemp
	
	u32Temp = au32TreeSection[u32I]
	au32TreeSection[u32I] = au32TreeSection[u32J]
	au32TreeSection[u32J] = u32Temp
	
	u32Temp = au32TreeMem[u32I]
	au32TreeMem[u32I] = au32TreeMem[u32J]
	au32TreeMem[u32J] = u32Temp
	
	u32Temp = au32TreeSize[u32I]
	au32TreeSize[u32I] = au32TreeSize[u32J]
	au32TreeSize[u32J] = u32Temp

	u32Temp = au32TreeLeaf[u32I]
	au32TreeLeaf[u32I] = au32TreeLeaf[u32J]
	au32TreeLeaf[u32J] = u32Temp
}

# Output html index 
function htmlIndex(bBreak) {
	printf "<p class=\"index\"><b>Index:</b> <a href=\"#top\">Top</a>, " > sPathHtml
	printf "<a href=\"#MEMORIES\">Memories</a>" > sPathHtml
	if (bBreak != 0) printf "<br/>" > sPathHtml
	else             printf " / " > sPathHtml
	printf "<a href=\"#%s\">%s</a> ", asMem[2], asMem[2] > sPathHtml
	if (bBreak != 0) printf "<br/>" > sPathHtml
	printf "[<a href=\"#%s-Banks\">Banks</a>, ", asMem[2] > sPathHtml
	printf "<a href=\"#%s-Sections\">Sections</a>, ", asMem[2]  > sPathHtml
	printf "<a href=\"#%s-Files\">Files</a>, ", asMem[2]  > sPathHtml
	printf "<a href=\"#%s-Symbols\">Symbols</a>, ", asMem[2]  > sPathHtml
	if (u32SourceFiles != 0) printf "<a href=\"#%s-Folders\">Folders</a>, ", asMem[2]  > sPathHtml
	if (u32SourceFiles != 0) printf "<a href=\"#%s-Tree\">Tree</a>, ", asMem[2]  > sPathHtml
	printf "<a href=\"#%s-Map\">Map</a>]", asMem[2] > sPathHtml
	if (bBreak != 0) printf "<br/>" > sPathHtml
	else             printf " / " > sPathHtml
	printf "<a href=\"#%s\">%s</a> ", asMem[1], asMem[1] > sPathHtml
	if (bBreak != 0) printf "<br/>" > sPathHtml
	printf "[<a href=\"#%s-Banks\">Banks</a>, ", asMem[1] > sPathHtml
	printf "<a href=\"#%s-Sections\">Sections</a>, ", asMem[1]  > sPathHtml
	printf "<a href=\"#%s-Files\">Files</a>, ", asMem[1]  > sPathHtml
	printf "<a href=\"#%s-Symbols\">Symbols</a>, ", asMem[1]  > sPathHtml
	if (u32SourceFiles != 0) printf "<a href=\"#%s-Folders\">Folders</a>, ", asMem[1]  > sPathHtml
	if (u32SourceFiles != 0) printf "<a href=\"#%s-Tree\">Tree</a>, ", asMem[1]  > sPathHtml
	printf "<a href=\"#%s-Map\">Map</a>]</p>\n", asMem[1]  > sPathHtml
}

# Output html memories
function htmlMemories() {
	sThisId = "MEMORIES"
	printf "<h2 id=\"%s\">Memories\n", sThisId > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h2>\n", sThisId > sPathHtml
	printf "<p>The Retain column displays the amount of data held during sleep for that memory.</p>\n" > sPathHtml
	printf "<p>The following memories are in the map file:</p>\n" > sPathHtml
	printf "<table>\n" > sPathHtml
	# Split table for new memory
	printf "<tr>\n" > sPathHtml
	printf "<th class=\"bg_xp\">Memory" > sPathHtml
	printf "</th>\n" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Start" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "End" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Size (kb)" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Size (b)" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Used" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Used %" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Retain (kb)" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Line" > sPathHtml
	printf "</tr>\n" > sPathHtml
	for (u32Mem = 1; u32Mem <= u32Mems; u32Mem++) {
		# Output mem details
		u32ThisPc = (au32MemUsed[u32Mem] * 100) / au32MemSize[u32Mem]
		if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
		printf "<tr>\n" > sPathHtml
		printf "<th class=\"left\">" > sPathHtml
		printf "%s", asMem[u32Mem] > sPathHtml
		printf "</th>\n" > sPathHtml
		printf "<td>0x%08x</td>\n",    au32MemAddr[u32Mem]  > sPathHtml
		printf "<td>0x%08x</td>\n",    au32MemEnd[u32Mem] > sPathHtml
		printf "<td>%dkb</td>\n",     (au32MemSize[u32Mem] / 1024) > sPathHtml
		printf "<td>%d</td>\n",        au32MemSize[u32Mem] > sPathHtml
		printf "<td>%d</td>\n",        au32MemUsed[u32Mem] > sPathHtml
		if (u32ThisPc > 100 || u32ThisPc < 0) printf "<td class=\"error\">%d%</td>\n", u32ThisPc > sPathHtml
		else                                  printf "<td>%d%</td>\n",                 u32ThisPc > sPathHtml
		printf "<td>%dkb</td>\n", (au32MemRetain[u32Mem] / 1024) > sPathHtml
		printf "<td>%d</td>\n",   au32MemLine[u32Mem]  > sPathHtml
		printf "</tr>\n" > sPathHtml
	}
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output html mem
function htmlMem(u32Mem) {
	sThisId = asMem[u32Mem]
	printf "<tr class=\"out_l\"><td class=\"out_l\" colspan=2><hr/>\n" > sPathHtml
	printf "<h2 id=\"%s\">%s", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h2>\n" > sPathHtml
	printf "<p>The Retain column displays the amount of data held during sleep.</p>\n" > sPathHtml
	printf "<p>Summary for the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	# Split table for new memory
	printf "<tr>\n" > sPathHtml
	printf "<th  class=\"bg_xp\">%s", asMem[u32Mem] > sPathHtml
	printf "</th>\n" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Start" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "End" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Size (kb)" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Size (b)" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Used" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Used %" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Retain (kb)" > sPathHtml
	printf "<th class=\"bg_xp\">%s</th>\n", "Line" > sPathHtml
	printf "</tr>\n" > sPathHtml
	# Output mem details
	u32ThisPc = (au32MemUsed[u32Mem] * 100) / au32MemSize[u32Mem]
	if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
	printf "<tr>\n" > sPathHtml
	printf "<th class=\"left\">" > sPathHtml
	printf "%s", asMem[u32Mem] > sPathHtml
	printf "</th>\n" > sPathHtml
	printf "<td>0x%08x</td>\n",    au32MemAddr[u32Mem]  > sPathHtml
	printf "<td>0x%08x</td>\n",    au32MemEnd[u32Mem] > sPathHtml
	printf "<td>%dkb</td>\n",     (au32MemSize[u32Mem] / 1024) > sPathHtml
	printf "<td>%d</td>\n",        au32MemSize[u32Mem] > sPathHtml
	printf "<td>%d</td>\n",        au32MemUsed[u32Mem] > sPathHtml
	if (u32ThisPc > 100 || u32ThisPc < 0) printf "<td class=\"error\">%d%</td>\n", u32ThisPc > sPathHtml
	else                                  printf "<td>%d%</td>\n",                 u32ThisPc > sPathHtml
	printf "<td>%dkb</td>\n",      (au32MemRetain[u32Mem] / 1024) > sPathHtml
	printf "<td>%d</td>\n",        au32MemLine[u32Mem]  > sPathHtml
	printf "</tr>\n" > sPathHtml
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
	printf "</td></tr>\n"  > sPathHtml
	printf "<tr class=\"out_l\">\n" > sPathHtml
	printf "<td class=\"out_l\">\n" > sPathHtml
	# Output banks
	htmlBanks(u32Mem)
	# Output sections
	htmlSections(u32Mem)
	# Output files
	htmlFiles(u32Mem)
	# Output symbols
	htmlSymbols(u32Mem)
	# Output folders
	if (u32SourceFiles != 0) htmlFolders(u32Mem)
	# Output tree
	if (u32SourceFiles != 0) htmlTree(u32Mem)
	printf "</td>\n"  > sPathHtml
	printf "<td class=\"out_r\">\n"  > sPathHtml
	# Output svg
	svgMemory(u32Mem)
	printf "</td>\n"  > sPathHtml
	printf "</tr>\n"  > sPathHtml
}

# Output html banks
function htmlBanks(u32Mem) {
	u32TotalUsed = 0
	u32TotalRetain = 0
	sThisId = asMem[u32Mem] "-Banks"
	printf "<h3 id=\"%s\">%s - Banks", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	# Bank description
	if (asMem[u32Mem] == "RAM0") {
		printf "<p>RAM0 is divided into physical memory banks of differing sizes. " > sPathHtml
		printf "When sleeping with RAM held different banks will be retained depending upon the linker configuration. " > sPathHtml
		printf "More power is consumed when more RAM is held during sleep.</p>\n" > sPathHtml
	}
	else if (asMem[u32Mem] == "ram") {
		printf "<p>The RAM memory is a single bank.\n" > sPathHtml
		printf "When sleeping with RAM held the whole memory bank is retained.</p>\n" > sPathHtml
	}
	else {
		printf "<p>The FLASH memory banks reflect the arrangement in a typical ZigBee 3.0 application.\n" > sPathHtml
		printf "These are logical banks rather than physical banks.\n" > sPathHtml
		printf "The data is useful for determining if the firmware fits into the bank allocated for the active software image.\n" > sPathHtml
		printf "If the inactive image bank is unused then the application is small enough to allow OTA operation.\n" > sPathHtml
	}
	printf "<p>The memory banks are not specified in the map file but are hardcoded into the analysis script.</p>\n" > sPathHtml
	printf "<p>The location and size of each bank is listed with the percentage of the memory used in that bank. " > sPathHtml
	printf "The sections present in each bank are also listed and may be split across multiple banks. When sleeping with RAM held the Retain column indicates how much data is retained.</p>\n" > sPathHtml
	printf "<p>Clicking the memory name in the header row will jump back to the memory information.</p>\n" > sPathHtml
	printf "<p>Clicking the a section name in the section column will jump forward to the section information.</p>\n" > sPathHtml
	printf "<p>The following banks are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	# Split table for new memory
	printf "<tr>\n" > sPathHtml
	sThisId = asMemHtml[u32Mem] "-Banks" > sPathHtml
	printf "<th class=\"bg_n\" id=\"%s\"><a href=\"#%s\">%s</a>", sThisId, asMem[u32Mem], asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</th>\n" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Start" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "End" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Size (kb)" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Size (b)" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Used" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Used %" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Retain (kb)"  > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Line" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Sections" > sPathHtml
	printf "</tr>\n" > sPathHtml
	for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
		if (u32Mem == au32BankMem[u32Bank]) {
			u32TotalUsed += au32BankUsed[u32Bank]
			u32TotalRetain += au32BankRetain[u32Bank]
			u32ThisPc = (au32BankUsed[u32Bank] * 100) / au32BankSize[u32Bank]
			if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			printf "<tr>\n" > sPathHtml
			printf "<th class=\"left\" id=\"%s\">", asBankHtml[u32Bank]  > sPathHtml
			printf "%s", asBank[u32Bank] > sPathHtml
			if (bOutputIds) printf " <small>(%s)</small>", asBankHtml[u32Bank] > sPathHtml
			printf "</th>\n" > sPathHtml
			printf "<td>0x%08x</td>\n",    au32BankAddr[u32Bank]  > sPathHtml
			printf "<td>0x%08x</td>\n",    au32BankEnd[u32Bank] > sPathHtml
			printf "<td>%dkb</td>\n",        (au32BankSize[u32Bank] / 1024) > sPathHtml
			printf "<td>%d</td>\n",        au32BankSize[u32Bank] > sPathHtml
			printf "<td>%d</td>\n",        au32BankUsed[u32Bank] > sPathHtml
			if (u32ThisPc > 100 || u32ThisPc < 0) printf "<td class=\"error\">%d%</td>\n", u32ThisPc > sPathHtml
			else                                  printf "<td>%d%</td>\n",                 u32ThisPc > sPathHtml
			printf "<td>%dkb</td>\n", (au32BankRetain[u32Bank] / 1024) > sPathHtml
			printf "<td>%d</td>\n",        au32BankLine[u32Bank]  > sPathHtml
			# Bank has sections in it ?  
			printf "<td class=\"left\">"  > sPathHtml
			if (au32BankSectionFirst[u32Bank] != 0) {
				sBreak = ""
				# Loop through sections in bank
				for (u32Section = au32BankSectionFirst[u32Bank]; u32Section <= au32BankSectionLast[u32Bank]; u32Section++) {
					printf "%s<a href=\"#%s\">%s</a>", sBreak, asSectionHtml[u32Section], asSection[u32Section] > sPathHtml
					sBreak = "<br/>"
				}
			}
			printf "</td>\n" > sPathHtml
			printf "<tr>\n" > sPathHtml
		}
	}
	sUsedClass = ""
	if (u32TotalUsed != au32MemUsed[u32Mem]) sUsedClass = " class=\"error\""
	# Calculate percentage of memory used 
	if (au32MemUsed[u32Mem] == 0) {
		u32ThisPc = -1
	}
	else {
		u32ThisPc = (u32TotalUsed * 100) / au32MemUsed[u32Mem]
		if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
	}
	sPcClass = ""
	if (u32ThisPc != 100) sPcClass = " class=\"error\""
	# Output total
	printf "<tr class=\"bg_ln\"><th class=\"bg_ln\" colspan=2>Expected used in <a href=\"#%s\">%s</a>:</th>\n", asMem[u32Mem], asMem[u32Mem] > sPathHtml
	printf "<th>%d</th>\n", au32MemUsed[u32Mem] > sPathHtml
	printf "<th class=\"bg_ln\" colspan=2>Actual:</th>\n"  > sPathHtml
	printf "<th%s>%d</th>\n", sUsedClass, u32TotalUsed > sPathHtml
	printf "<th%s>%d%%</th>\n",    sPcClass,   u32ThisPc > sPathHtml
	printf "<th>%dkb</th>\n", (u32TotalRetain / 1024) > sPathHtml
	printf "<th>%d</th>\n", au32MemLine[u32Mem] > sPathHtml
	printf "<th></th></tr>\n" > sPathHtml
	# Reset total
	u32TotalUsed = 0
	# OUTPUT TABLE ROW HERE
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output html sections
function htmlSections(u32Mem) {
	u32TotalUsed = 0
	sThisId = asMem[u32Mem] "-Sections"
	printf "<h3 id=\"%s\">%s - Sections", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	printf "<p>The linker divides allocated memory into sections. " > sPathHtml
	printf "The location and size of each section is listed with the percentage of the total used memory that section occupies. " > sPathHtml
	printf "The bank(s) used by each section are also listed.</p>\n" > sPathHtml
	printf "<p>Clicking the memory name in the header row will jump back to the memory information.</p>\n" > sPathHtml
	printf "<p>Clicking a bank name in the Banks column will jump back to the bank information.</p>\n" > sPathHtml
	printf "<p>Clicking the a section name in the section column will jump forward to that section's files.</p>\n" > sPathHtml
	printf "<p>The following sections are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	# Split table for new memory
	printf "<tr class=\"bg_n\">\n" > sPathHtml
	sThisId = asMemHtml[u32Mem] "-Sections" > sPathHtml
	printf "<th id=\"%s\" class=\"left\"><a href=\"#%s\">%s</a>", sThisId, asMem[u32Mem], asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</th>\n" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Start" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "End" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Size" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Used" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Used %" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Line" > sPathHtml
	printf "<th class=\"bg_n\">%s</th>\n", "Banks" > sPathHtml
	printf "</tr>\n" > sPathHtml
	for (u32Section = 1; u32Section <= u32Sections; u32Section++) { 
		if (u32Mem == au32SectionMem[u32Section]) {
			u32TotalUsed += au32SectionSize[u32Section]
			u32ThisPc = (au32SectionSize[u32Section] * 100) / au32MemUsed[u32Mem]
			if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			printf "<tr>\n" > sPathHtml
			printf "<th class=\"left\" id=\"%s\">", asSectionHtml[u32Section] > sPathHtml
			if (au32SectionFiles[u32Section] > 0) {
				sThisHref = asSectionHtml[u32Section] "-Files"
				printf "<a href=\"#%s\">%s</a>", sThisHref, asSection[u32Section] > sPathHtml
			}
			else {
				printf "%s", asSection[u32Section] > sPathHtml
			}
			if (bOutputIds) printf " <small>(%s)</small>", asSectionHtml[u32Section] > sPathHtml
			printf "</th>\n" > sPathHtml
			printf "<td>0x%08x</td>\n",    au32SectionAddr[u32Section]  > sPathHtml
			printf "<td>0x%08x</td>\n",    au32SectionEnd[u32Section] > sPathHtml
			printf "<td>%d</td>\n",        au32SectionSize[u32Section] > sPathHtml
			printf "<td>%d</td>\n",        au32SectionUsed[u32Section] > sPathHtml
			if (u32ThisPc > 100 || u32ThisPc < 0) printf "<td class=\"error\">%d%</td>\n", u32ThisPc > sPathHtml
			else                                  printf "<td>%d%</td>\n",                 u32ThisPc > sPathHtml
			printf "<td>%d</td>\n",        au32SectionLine[u32Section]  > sPathHtml
			# Section has banks in it ? 
			printf "<td class=\"left\">" > sPathHtml
			if (au32SectionBankFirst[u32Section] != 0) {
				sBreak = ""
				# Loop through banks in section
				for (u32Bank = au32SectionBankFirst[u32Section]; u32Bank <= au32SectionBankLast[u32Section]; u32Bank++) {
					printf "%s<a href=\"#%s\">%s</a>", sBreak, asBankHtml[u32Bank], asBank[u32Bank] > sPathHtml
					sBreak = "<br/>"
				}
			}
			printf "</td>\n" > sPathHtml
			printf "<tr>\n" > sPathHtml
		}
	}
	sUsedClass = ""
	if (u32TotalUsed != au32MemUsed[u32Mem]) sUsedClass = " class=\"error\""
	# Calculate percentage of memory used 
	if (au32MemUsed[u32Mem] == 0) {
		u32ThisPc = -1
	}
	else {
		u32ThisPc = (u32TotalUsed * 100) / au32MemUsed[u32Mem]
		if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
	}
	sPcClass = ""
	if (u32ThisPc != 100) sPcClass = " class=\"error\""
	# Output total
	printf "<tr class=\"bg_ln\"><th class=\"bg_ln\" colspan=2>Expected used in <a href=\"#%s\">%s</a>:</th>\n", asMem[u32Mem], asMem[u32Mem]  > sPathHtml
	printf "<th>%d</th>\n", au32MemUsed[u32Mem] > sPathHtml
	printf "<th class=\"bg_ln\">Actual:</th>\n"  > sPathHtml
	printf "<th%s>%d</th>\n", sUsedClass, u32TotalUsed > sPathHtml
	printf "<th%s>%d%%</th>\n",    sPcClass,   u32ThisPc > sPathHtml
	printf "<th>%d</th>\n", au32MemLine[u32Mem] > sPathHtml
	printf "<th></th></tr>\n" > sPathHtml
	# Reset total
	u32TotalUsed = 0
	# OUTPUT TABLE ROW HERE
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output html files
function htmlFiles(u32Mem) {
	u32FileCount = 0
	u32TotalUsed = 0
	sThisId = asMem[u32Mem] "-Files"
	printf "<h3 id=\"%s\">%s - Files", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	printf "<p>The memory used by each file is listed below, grouped by section. " > sPathHtml
	printf "The percentage column is the percentage of the section memory that file occupies.</p>\n" > sPathHtml
	printf "<p>Clicking the memory or section name in a header row will jump back to the memory or section information.</p>\n" > sPathHtml
	printf "<p>Clicking a file name in the file column will jump forward to that file's symbols.</p>\n" > sPathHtml
	printf "<p>The following files are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	for (u32File = 1; u32File <= u32Files; u32File++) {
		if (u32Mem == au32FileMem[u32File]) {
			# Is this a different section to the previous file
			if (u32FileCount == 0 || au32FileSection[u32File] != au32FileSection[u32File-1]) {
				# Split table for new section
				sThisId = asSectionHtml[au32FileSection[u32File]] "-Files"
				printf "<tr><th class=\"bg_x\" id=\"%s\"><a href=\"#%s\">%s</a>", sThisId, asMem[au32FileMem[u32File]], asMem[au32FileMem[u32File]] > sPathHtml
				printf " / <a href=\"#%s\">%s</a>", asSectionHtml[au32FileSection[u32File]], asSection[au32FileSection[u32File]] > sPathHtml
				if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
				printf "</th>\n"  > sPathHtml
				printf "<th class=\"bg_x\">%s</th>\n", "Start" > sPathHtml
				printf "<th class=\"bg_x\">%s</th>\n", "End" > sPathHtml
				printf "<th class=\"bg_x\">%s</th>\n", "Size" > sPathHtml
				printf "<th class=\"bg_x\">%s</th>\n", "%" > sPathHtml
				printf "<th class=\"bg_x\">%s</th>\n", "Line" > sPathHtml
				printf "</tr>\n" > sPathHtml
			}
			u32TotalUsed += au32FileSize[u32File]
			# Calculate percentage of file used by this file
			if (au32SectionSize[au32FileSection[u32File]] == 0) {
				u32ThisPc = -1
			}
			else {
				u32ThisPc = (au32FileSize[u32File] * 100) / au32SectionSize[au32FileSection[u32File]]
				if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			}
			sPcClass = ""
			if (u32ThisPc < 1 || u32ThisPc > 100) sPcClass = " class=\"error\""
			# Calculate if address overlaps previous file
			sAddrClass = ""
			if (u32FileCount > 0) {
				# Overlaid with previous file ?
				if (au32FileAddr[u32File] < au32FileEnd[u32File-1]) {
					# Set class
					sAddrClass = " class=\"warning\""
				}
			}
			# Calculate if end does not match address of next file
			sEndClass = ""
			if (u32File < u32Files) {
				if (au32FileBank[u32File]   == au32FileBank[u32File+1] ||
					au32FileBank[u32File]+1 == au32FileBank[u32File+1]) {
					# Overlaid with previous file ?
					if (au32FileEnd[u32File] > au32FileAddr[u32File+1]) {
						# Set class
						sEndClass = " class=\"warning\""
					}
				}
			}
			printf "<tr>\n" > sPathHtml
			sThisHref = asFileHtml[u32File] "-Symbols" > sPathHtml
			printf "<th class=\"left\" id=\"%s\"><a href=\"#%s\">%s</a>", asFileHtml[u32File], sThisHref, asFile[u32File] > sPathHtml
			if (bOutputIds) printf " <small>(%s)</small>", asFileHtml[u32File] > sPathHtml
			printf "</th>\n" > sPathHtml
			printf "<td%s>0x%08x</td>\n",  sAddrClass, au32FileAddr[u32File]  > sPathHtml
			printf "<td%s>0x%08x</td>\n",    sEndClass,  au32FileEnd[u32File] > sPathHtml
			printf "<td%s>%d</td>\n",      sPcClass,   au32FileSize[u32File] > sPathHtml
			printf "<td%s>%d%%</td>\n",    sPcClass,   u32ThisPc > sPathHtml
			printf "<td>%d</td>\n",        au32FileLine[u32File]  > sPathHtml
			printf "<tr>\n" > sPathHtml
			# Is this a different section to the next
			if (au32FileSection[u32File] != au32FileSection[u32File+1]) {
				sUsedClass = ""
				if (u32TotalUsed != au32SectionSize[au32FileSection[u32File]]) sUsedClass = " class=\"error\""
				# Calculate percentage of file used 
				if (au32SectionSize[au32FileSection[u32File]] == 0) {
					u32ThisPc = -1
				}
				else {
					u32ThisPc = (u32TotalUsed * 100) / au32SectionSize[au32FileSection[u32File]]
					if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
				}
				sPcClass = ""
				if (u32ThisPc != 100) sPcClass = " class=\"error\""
				# Output total
				printf "<tr class=\"bg_lx\"><th class=\"bg_lx\">Expected total in <a href=\"#%s\">%s</a> section:</th>\n", asSectionHtml[au32FileSection[u32File]], asSection[au32FileSection[u32File]]  > sPathHtml
				printf "<th>%d</th>\n", au32SectionSize[au32FileSection[u32File]] > sPathHtml
				printf "<th class=\"bg_lx\">Actual:</th>\n"  > sPathHtml
				printf "<th%s>%d</th>\n", sUsedClass, u32TotalUsed > sPathHtml
				printf "<th%s>%d%%</th>\n",    sPcClass,   u32ThisPc > sPathHtml
				printf "<th>%d</th></tr>\n", au32SectionLine[au32FileSection[u32File]] > sPathHtml
				# Reset total
				u32TotalUsed = 0
			}
			u32FileCount++
		}
	}
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output html symbols
function htmlSymbols(u32Mem) {
	u32SymbolCount = 0
	u32TotalUsed = 0
	sThisId = asMem[u32Mem] "-Symbols"
	printf "<h3 id=\"%s\">%s - Symbols", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	printf "<p>The memory used by each symbol is listed below, grouped by file. " > sPathHtml
	printf "The percentage column is the percentage of the file memory that symbol occupies.</p>\n" > sPathHtml
	printf "<p>Clicking the memory, section or file name in a header row will jump back to the memory, section or file information.</p>\n" > sPathHtml
	printf "<p>The following symbols are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	for (u32Symbol = 1; u32Symbol <= u32Symbols; u32Symbol++) {
		if (u32Mem == au32SymbolMem[u32Symbol]) {
			# Is this a different file to the previous symbol
			if (u32SymbolCount == 0 || au32SymbolFile[u32Symbol] != au32SymbolFile[u32Symbol-1]) {
				# Split heirarchy
				sThisId = asFileHtml[au32SymbolFile[u32Symbol]] "-Symbols"
				printf "<tr><th class=\"bg_p\" id=\"%s\"><a href=\"#%s\">%s</a>", sThisId, asMem[au32SymbolMem[u32Symbol]], asMem[au32SymbolMem[u32Symbol]]   > sPathHtml
				printf " / <a href=\"#%s\">%s</a>", asSectionHtml[au32SymbolSection[u32Symbol]], asSection[au32SymbolSection[u32Symbol]]  > sPathHtml
				printf " / <a href=\"#%s\">%s</a>", asFileHtml[au32SymbolFile[u32Symbol]], asFile[au32SymbolFile[u32Symbol]] > sPathHtml
				if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
				printf "</th>\n" > sPathHtml
				# Output column headers
				printf "<th class=\"bg_p\">%s</th>\n", "Start" > sPathHtml
				printf "<th class=\"bg_p\">%s</th>\n", "End" > sPathHtml
				printf "<th class=\"bg_p\">%s</th>\n", "Size" > sPathHtml
				printf "<th class=\"bg_p\">%s</th>\n", "%" > sPathHtml
				printf "<th class=\"bg_p\">%s</th>\n", "Line" > sPathHtml
				printf "</tr>\n" > sPathHtml
			}
			u32TotalUsed += au32SymbolSize[u32Symbol]
			# Calculate percentage of file used by this symbol
			if (au32FileSize[au32SymbolFile[u32Symbol]] == 0) {
				u32ThisPc = -1
			}
			else {
				u32ThisPc = (au32SymbolSize[u32Symbol] * 100) / au32FileSize[au32SymbolFile[u32Symbol]]
				if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			}
			sPcClass = ""
			if (u32ThisPc < 1 || u32ThisPc > 100) sPcClass = " class=\"error\""
			# Calculate if address overlaps previous symbol
			sAddrClass = ""
			if (u32SymbolCount > 0) {
				# Overlaid with previous symbol ?
				if (au32SymbolAddr[u32Symbol] < au32SymbolEnd[u32Symbol-1]) {
					# Set class
					sAddrClass = " class=\"warning\""
				}
			}
			# Calculate if end does not match address of next symbol
			sEndClass = ""
			if (u32Symbol < u32Symbols) {
				if (au32SymbolBank[u32Symbol]   == au32SymbolBank[u32Symbol+1] ||
					au32SymbolBank[u32Symbol]+1 == au32SymbolBank[u32Symbol+1]) {
					# Overlaid with previous symbol ?
					if (au32SymbolEnd[u32Symbol] > au32SymbolAddr[u32Symbol+1]) {
						# Set class
						sEndClass = " class=\"warning\""
					}
				}
			}
			printf "<tr>\n" > sPathHtml
			printf "<td class=\"left\" id=\"%s\">%s", asSymbolHtml[u32Symbol], asSymbol[u32Symbol] > sPathHtml
			if (bOutputIds) printf " <small>(%s)</small>", asSymbolHtml[u32Symbol] > sPathHtml
			printf "</td>\n" > sPathHtml
			printf "<td%s>0x%08x</td>\n",  sAddrClass, au32SymbolAddr[u32Symbol]  > sPathHtml
			printf "<td%s>0x%08x</td>\n",    sEndClass,  au32SymbolEnd[u32Symbol] > sPathHtml
			printf "<td%s>%d</td>\n",      sPcClass,   au32SymbolSize[u32Symbol] > sPathHtml
			printf "<td%s>%d%%</td>\n",    sPcClass,   u32ThisPc > sPathHtml
			printf "<td>%d</td>\n",        au32SymbolLine[u32Symbol]  > sPathHtml
			printf "<tr>\n" > sPathHtml
			# Is this a different file to the next
			if (au32SymbolFile[u32Symbol] != au32SymbolFile[u32Symbol+1]) {
				sUsedClass = ""
				if (u32TotalUsed != au32FileSize[au32SymbolFile[u32Symbol]]) sUsedClass = " class=\"error\""
				# Calculate percentage of file used 
				if (au32FileSize[au32SymbolFile[u32Symbol]] == 0) {
					u32ThisPc = -1
				}
				else {
					u32ThisPc = (u32TotalUsed * 100) / au32FileSize[au32SymbolFile[u32Symbol]]
					if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
				}
				sPcClass = ""
				if (u32ThisPc != 100) sPcClass = " class=\"error\""
				# Output total
				printf "<tr class=\"bg_lp\"><th class=\"bg_lp\">Expected total in <a href=\"#%s\">%s</a> file:</th>\n", asFileHtml[au32SymbolFile[u32Symbol]], asFile[au32SymbolFile[u32Symbol]] > sPathHtml
				printf "<th>%d</th>\n", au32FileSize[au32SymbolFile[u32Symbol]] > sPathHtml
				printf "<th class=\"bg_lp\">Actual:</th>\n"  > sPathHtml
				printf "<th%s>%d</th>\n", sUsedClass, u32TotalUsed > sPathHtml
				printf "<th%s>%d%%</th>\n",    sPcClass,   u32ThisPc > sPathHtml
				printf "<th>%d</th></tr>\n", au32FileLine[au32SymbolFile[u32Symbol]] > sPathHtml
				# Reset total
				u32TotalUsed = 0
			}
			u32SymbolCount++
		}
	}
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output html folders
function htmlFolders(u32Mem) {
	u32FolderCount = 0
	u32TotalUsed = 0
	sThisId = asMem[u32Mem] "-Folders"
	printf "<h3 id=\"%s\">%s - Folders", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	printf "<p>The memory used by each folder is listed below, grouped by section. " > sPathHtml
	printf "The sizes and percentages are for the contents of that folder only, (so contents of subfolders are not included).\n" > sPathHtml
	printf "The percentage column is the percentage of the section memory that Folder occupies.</p>\n" > sPathHtml
	printf "<p>The following folders are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	for (u32Folder = 1; u32Folder <= u32Folders; u32Folder++) {
		if (u32Mem == au32FolderMem[u32Folder]) {
			# Is this a different section to the previous folder
			if (u32FolderCount == 0 || au32FolderSection[u32Folder] != au32FolderSection[u32Folder-1]) {
				# Split table for new section
				sThisId = asSectionHtml[au32FolderSection[u32Folder]] "-Folders"
				printf "<tr><th class=\"bg_nx\" id=\"%s\" colspan=2><a href=\"#%s\">%s</a>", sThisId, asMem[au32FolderMem[u32Folder]], asMem[au32FolderMem[u32Folder]] > sPathHtml
				printf " / <a href=\"#%s\">%s</a>", asSectionHtml[au32FolderSection[u32Folder]], asSection[au32FolderSection[u32Folder]] > sPathHtml
				if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
				printf "</th>\n"  > sPathHtml
				printf "<th class=\"bg_nx\">%s</th>\n", "Files" > sPathHtml
				printf "<th class=\"bg_nx\">%s</th>\n", "Size" > sPathHtml
				printf "<th class=\"bg_nx\">%s</th>\n", "%" > sPathHtml
				printf "</tr>\n" > sPathHtml
			}
			u32TotalUsed += au32FolderSize[u32Folder]
			# Calculate percentage of Folder used by this Folder
			if (au32SectionSize[au32FolderSection[u32Folder]] == 0) {
				u32ThisPc = -1
			}
			else {
				u32ThisPc = (au32FolderSize[u32Folder] * 100) / au32SectionSize[au32FolderSection[u32Folder]]
				if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			}
			sPcClass = ""
			if (u32ThisPc < 1 || u32ThisPc > 100) sPcClass = " class=\"error\""
			printf "<tr>\n" > sPathHtml
			sThisHref = asFolderHtml[u32Folder] "-Symbols"
			printf "<th class=\"left\" id=\"%s\" colspan=2>%s/", asFolderHtml[u32Folder], asFolder[u32Folder] > sPathHtml
			if (bOutputIds) printf " <small>(%s)</small>", asFolderHtml[u32Folder] > sPathHtml
			printf "</th>\n" > sPathHtml
			printf "<td>%d</td>\n",                    au32FolderFiles[u32Folder] > sPathHtml
			printf "<td%s>%d</td>\n",      sPcClass,   au32FolderSize[u32Folder] > sPathHtml
			printf "<td%s>%d%%</td>\n",    sPcClass,   u32ThisPc > sPathHtml
			printf "<tr>\n" > sPathHtml
			# Is this a different section to the next
			if (au32FolderSection[u32Folder] != au32FolderSection[u32Folder+1]) {
				sUsedClass = ""
				if (u32TotalUsed != au32SectionSize[au32FolderSection[u32Folder]]) sUsedClass = " class=\"error\""
				# Calculate percentage of Folder used 
				if (au32SectionSize[au32FolderSection[u32Folder]] == 0) {
					u32ThisPc = -1
				}
				else {
					u32ThisPc = (u32TotalUsed * 100) / au32SectionSize[au32FolderSection[u32Folder]]
					if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
				}
				sPcClass = ""
				if (u32ThisPc != 100) sPcClass = " class=\"error\""
				# Output total
				printf "<tr class=\"bg_ln\"><th class=\"bg_lx\">Expected total in <a href=\"#%s\">%s</a> section:</th>\n", asSectionHtml[au32FolderSection[u32Folder]], asSection[au32FolderSection[u32Folder]]  > sPathHtml
				printf "<th>%d</th>\n", au32SectionSize[au32FolderSection[u32Folder]] > sPathHtml
				printf "<th class=\"bg_ln\">Actual:</th>\n"  > sPathHtml
				printf "<th%s>%d</th>\n", sUsedClass, u32TotalUsed > sPathHtml
				printf "<th%s>%d%%</th></tr>\n",    sPcClass,   u32ThisPc > sPathHtml
				# Reset total
				u32TotalUsed = 0
			}
			u32FolderCount++
		}
	}
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output html tree
function htmlTree(u32Mem) {
	u32TreeCount = 0
	sThisId = asMem[u32Mem] "-Tree"
	printf "<h3 id=\"%s\">%s - Tree", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	printf "<p>The memory used by each folder and file is listed below, grouped by section." > sPathHtml
	printf "The sizes given for folders include the contents of that folder and all subfolders. " > sPathHtml
	printf "The percentage column is the percentage of the section memory that entry occupies.</p>\n" > sPathHtml
#	printf "<p>Clicking the memory or section name in a header row will jump back to the memory or section information.</p>\n" > sPathHtml
#	printf "<p>Clicking a Folder name in the Folder column will jump forward to that Folder's symbols.</p>\n" > sPathHtml
	printf "<p>The following folders and files are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	printf "<table>\n" > sPathHtml
	for (u32Tree = 1; u32Tree <= u32Trees; u32Tree++) {
		if (u32Mem == au32TreeMem[u32Tree]) {
			# Is this a different section to the previous tree
			if (u32TreeCount == 0 || au32TreeSection[u32Tree] != au32TreeSection[u32Tree-1]) {
				# Split table for new section
				sThisId = asSectionHtml[au32FolderSection[u32Folder]] "-Tree"
				printf "<tr><th class=\"bg_nx\" id=\"%s\" colspan=2><a href=\"#%s\">%s</a>", sThisId, asMem[au32TreeMem[u32Tree]], asMem[au32TreeMem[u32Tree]] > sPathHtml
				printf " / <a href=\"#%s\">%s</a>", asSectionHtml[au32TreeSection[u32Tree]], asSection[au32TreeSection[u32Tree]] > sPathHtml
				if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
				printf "</th>\n"  > sPathHtml
				printf "<th class=\"bg_nx\">%s</th>\n", "Size" > sPathHtml
				printf "<th class=\"bg_nx\">%s</th>\n", "%" > sPathHtml
				printf "</tr>\n" > sPathHtml
			}
			# Calculate percentage of Folder used by this Folder
			if (au32SectionSize[au32TreeSection[u32Tree]] == 0) {
				u32ThisPc = -1
			}
			else {
				u32ThisPc = (au32TreeSize[u32Tree] * 100) / au32SectionSize[au32TreeSection[u32Tree]]
				if (int(u32ThisPc) != u32ThisPc) u32ThisPc = int(u32ThisPc)+1
			}
			sPcClass = ""
			if (u32ThisPc < 1 || u32ThisPc > 100) sPcClass = " class=\"error\""
			printf "<tr>\n" > sPathHtml
			printf "<td class=\"left\" id=\"%s\" colspan=2>%s", asTreeHtml[u32Tree], asTreeDisplay[u32Tree] > sPathHtml
			if (bOutputIds) printf " <small>(%s)</small>", asTreeHtml[u32Tree] > sPathHtml
			printf "</td>\n" > sPathHtml
			printf "<td%s>%d</td>\n",      sPcClass,   au32TreeSize[u32Tree] > sPathHtml
			printf "<td%s>%d%%</td>\n",    sPcClass,   u32ThisPc > sPathHtml
			printf "<tr>\n" > sPathHtml
			u32TreeCount++
		}
	}
	printf "</table>\n" > sPathHtml
	# Output index
	htmlIndex()
}

# Output svg memory
function svgMemory(u32Mem) {
	sThisId = asMem[u32Mem] "-Map"
	printf "<h3 id=\"%s\">%s - Map", sThisId, asMem[u32Mem] > sPathHtml
	if (bOutputIds) printf " <small>(%s)</small>", sThisId > sPathHtml
	printf "</h3>\n" > sPathHtml
	printf "<p>Tooltips indicate block information.</p>\n" > sPathHtml
	printf "<p>Click a block to jump to its information.</p>\n" > sPathHtml
	printf "<p>The following banks, sections and files are in the %s memory:</p>\n", asMem[u32Mem] > sPathHtml
	# END - Image Initialise ----------------------------------------------------------------------
	u32FontSizePx  = 26
	u32Scale       = 2
	u32ImageWidth  = 256
	u32ImageHeight = 0
	u32ImageAddr   = -1
	u32ImageSize   = 0
	# Loop though banks
	for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
		# Is this bank used ?
		if (au32BankMem[u32Bank] == u32Mem) {
		    if (au32BankUsed[u32Bank] > 0) {
				if (u32ImageAddr == -1) u32ImageAddr = au32BankAddr[u32Bank]
				u32ImageSize += au32BankSize[u32Bank]
			}
			else {
				for (u32Bank2 = u32Bank+1; u32Bank2 <= u32Banks; u32Bank2++) {
					if (au32BankMem[u32Bank2] == u32Mem) {
						au32BankAdjust[u32Bank2] += au32BankSize[u32Bank]
					}
				}
			}
		}
	}
	# Calculate scale
	#if      (u32ImageSize <=  16384) u32Scale = 1
	if      (u32ImageSize <=  32768) u32Scale = 1			 
	else if (u32ImageSize <=  65536) u32Scale = 2
	else if (u32ImageSize <= 131072) u32Scale = 4
	else if (u32ImageSize <= 262144) u32Scale = 8
	else if (u32ImageSize <= 524288) u32Scale = 16
	else                             u32Scale = 32
	# Calculate image height
	u32ImageHeight = (u32ImageSize / u32Scale) + 2
	printf "<!-- SVG - HEAD ------------------------------------------------------------------->\n" > sPathHtml
	printf "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" width=\"%d\" height=\"%d\">\n", 
		u32ImageWidth,
		u32ImageHeight > sPathHtml
	printf "<!-- SVG - BANKS ------------------------------------------------------------------>\n" > sPathHtml
	u32X = 1
	u32Width = u32ImageWidth - 2
	for (u32Bank = 1; u32Bank <= u32Banks; u32Bank++) {
		if (au32BankMem[u32Bank] == u32Mem && au32BankUsed[u32Bank] != 0) {
			u32Y      = (((au32BankAddr[u32Bank] - au32BankAdjust[u32Bank] - u32ImageAddr) / u32Scale) + 1)
			u32Height =   (au32BankSize[u32Bank]                 / u32Scale)
			printf "<a xlink:href=\"#%s\">\n", asBankHtml[u32Bank] > sPathHtml
			printf "<rect x=\"%d\" width=\"%d\" y=\"%d\" height=\"%d\" stroke-width=\"2\" stroke=\"%s\" fill=\"%s\"><title>#%d BANK: %s - Start=0x%08x End=0x%08x Size=%d</title></rect>\n",
				u32X,
				u32Width,
				u32Y,
				u32Height,
				sDarkN,
				sColorN,
				au32BankLine[u32Bank],
				asBank[u32Bank],
				au32BankAddr[u32Bank],
				au32BankEnd[u32Bank],
				au32BankSize[u32Bank] > sPathHtml
			printf "<text x=\"%d\" y=\"%d\" transform=\"rotate(90, %d, %d)\" style=\"font-family: Arial; font-size: %dpx; \"><title>BANK: %s - Start=0x%08x End=0x%08x Size=%d</title>%s</text>\n",
				u32X + 8,
				u32Y + 2,
				u32X + 8,
				u32Y + 2,
				u32FontSizePx,
				asBank[u32Bank],
				au32BankAddr[u32Bank],
				au32BankEnd[u32Bank],
				au32BankSize[u32Bank],
				asBank[u32Bank] > sPathHtml
			printf "</a>\n" > sPathHtml
		}
	}
	printf "<!-- SVG - SECTIONS --------------------------------------------------------------->\n" > sPathHtml
	if (1) {
		u32X     += 32
		u32Width -= 32
		for (u32Section = 1; u32Section <= u32Sections; u32Section++) {
			if (au32SectionMem[u32Section] == u32Mem && au32SectionSize[u32Section] > 0) {
				u32Y      = (((au32SectionAddr[u32Section] - au32BankAdjust[au32SectionBank[u32Section]] - u32ImageAddr) / u32Scale) + 1)
				u32Height =   (au32SectionSize[u32Section]                 / u32Scale)
				printf "<a xlink:href=\"#%s\">\n", asSectionHtml[u32Section] > sPathHtml
				printf "<rect x=\"%d\" width=\"%d\" y=\"%d\" height=\"%d\" stroke-width=\"2\" stroke=\"%s\" fill=\"%s\"><title>#%d SECTION: %s - Start=0x%08x End=0x%08x Size=%d</title></rect>\n",
					u32X,
					u32Width,
					u32Y,
					u32Height,
					sDarkX,
					sColorX,
					au32SectionLine[u32Section],
					asSection[u32Section],
					au32SectionAddr[u32Section],
					au32SectionEnd[u32Section],
					au32SectionSize[u32Section] > sPathHtml
				if (au32SectionFile[u32Section] != 0) {				
					printf "<text x=\"%d\" y=\"%d\" transform=\"rotate(90, %d, %d)\" style=\"font-family: Arial; font-size: %dpx; \"><title>#%d SECTION: %s - Start=0x%08x End=0x%08x Size=%d</title>%s</text>\n",
						u32X + 8,
						u32Y + 2,
						u32X + 8,
						u32Y + 2,
						u32FontSizePx,
						au32SectionLine[u32Section],
						asSection[u32Section],
						au32SectionAddr[u32Section],
						au32SectionEnd[u32Section],
						au32SectionSize[u32Section],
						asSection[u32Section] > sPathHtml
				}
				else if (u32Height > u32FontSizePx) {
					printf "<text x=\"%d\" y=\"%d\" style=\"font-family: Arial; font-size: %dpx; \"><title>#%d SECTION: %s - Start=0x%08x End=0x%08x Size=%d</title>%s</text>\n",
						u32X + 8,
						u32Y + u32FontSizePx - 2,
						u32FontSizePx,
						au32SectionLine[u32Section],
						asSection[u32Section],
						au32SectionAddr[u32Section],
						au32SectionEnd[u32Section],
						au32SectionSize[u32Section],
						asSection[u32Section] > sPathHtml
				}
				printf "</a>\n" > sPathHtml
			}
		}
	}	
	printf "<!-- SVG - FILES ------------------------------------------------------------------>\n" > sPathHtml
	if (1) {
		u32FontSizePx = 14
		u32X += 32
		u32Width -= 32
		for (u32File = 1; u32File <= u32Files; u32File++) {
			if (au32FileMem[u32File] == u32Mem) {
				u32Y      = (((au32FileAddr[u32File] - au32BankAdjust[au32FileBank[u32File]] - u32ImageAddr) / u32Scale) + 1)
				u32Height =   (au32FileSize[u32File]                 / u32Scale)
				printf "<a xlink:href=\"#%s\">\n", asFileHtml[u32File] > sPathHtml
				printf "<rect x=\"%d\" width=\"%d\" y=\"%d\" height=\"%d\" stroke-width=\"2\" stroke=\"%s\" fill=\"%s\"><title>#%d FILE: %s - Start=0x%08x End=0x%08x Size=%d</title></rect>\n",
					u32X,
					u32Width,
					u32Y,
					u32Height,
					sDarkP,
					sColorP,
					au32FileLine[u32File],
					asFile[u32File],
					au32FileAddr[u32File],
					au32FileEnd[u32File],
					au32FileSize[u32File] > sPathHtml
				if (u32Height > u32FontSizePx + 3) {
					printf "<text x=\"%d\" y=\"%d\" style=\"font-family: Arial; font-size: %dpx;\"><title>#%d FILE: %s - Start=0x%08x End=0x%08x Size=%d</title>%s</text>\n",
						(u32X + 3),
						(u32Y + u32FontSizePx - 1),
						u32FontSizePx,
						au32FileLine[u32File],
						asFile[u32File],
						au32FileAddr[u32File],
						au32FileEnd[u32File],
						au32FileSize[u32File],
						asFile[u32File] > sPathHtml
				}
				printf "</a>\n" > sPathHtml
			}
		}
	}
	# Memory close svg
	printf "</svg>\n" > sPathHtml
	# Output index
	htmlIndex(1)
}

