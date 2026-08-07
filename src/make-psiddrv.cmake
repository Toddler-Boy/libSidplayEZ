# Rebuilds psiddrv.bin (hex dump of the relocatable o65 driver) from psiddrv.a65.
# Needs cc65 on PATH, run: cmake -P make-psiddrv.cmake
# The two ld65 "Cannot evaluate assertion" warnings are ca65's jmp-vector page
# checks, unevaluable in relocatable output and known safe here.

set(here ${CMAKE_CURRENT_LIST_DIR})

find_program(CA65 ca65)
find_program(LD65 ld65)
if(NOT CA65 OR NOT LD65)
	message(WARNING "cc65 not on PATH, psiddrv.bin NOT rebuilt from psiddrv.a65")
	return()
endif()

# the memory bases mirror what xa wrote into the o65 header, keeping the
# regenerated file byte-identical to the previous pipeline's output
file(WRITE ${here}/psiddrv.tmp.cfg [=[
MEMORY {
    ZP:   start = $0004, size = $00FC, type = rw;
    MAIN: start = $1000, size = $3000, file = %O;
    DRAM: start = $0400, size = $0C00, file = %O;
    BRAM: start = $4000, size = $1000, file = %O;
}
SEGMENTS {
    ZEROPAGE: load = ZP,   type = zp,  optional = yes;
    CODE:     load = MAIN, type = ro;
    DATA:     load = DRAM, type = rw,  optional = yes;
    BSS:      load = BRAM, type = bss, optional = yes;
}
FILES {
    %O: format = o65;
}
]=])

execute_process(COMMAND ${CA65} psiddrv.a65 -o psiddrv.o
				WORKING_DIRECTORY ${here} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "ca65 failed")
endif()

execute_process(COMMAND ${LD65} -C psiddrv.tmp.cfg -o psiddrv.o65 psiddrv.o
				WORKING_DIRECTORY ${here} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "ld65 failed")
endif()

file(READ ${here}/psiddrv.o65 hex HEX)
file(REMOVE ${here}/psiddrv.o ${here}/psiddrv.o65 ${here}/psiddrv.tmp.cfg)

# drop the o65 header options (linker version, timestamp) for deterministic output
string(SUBSTRING ${hex} 0 52 head)
set(pos 52)
string(SUBSTRING ${hex} ${pos} 2 len)
while(NOT len STREQUAL "00")
	math(EXPR pos "${pos} + 2 * 0x${len}")
	string(SUBSTRING ${hex} ${pos} 2 len)
endwhile()
math(EXPR pos "${pos} + 2")
string(SUBSTRING ${hex} ${pos} -1 rest)
set(hex "${head}00${rest}")

# emit in the od -w8 layout the old xa pipeline produced
string(REGEX MATCHALL ".." pairs ${hex})
set(bin "")
set(line "")
set(n 0)
foreach(p IN LISTS pairs)
	string(APPEND line " 0x${p},")
	math(EXPR n "${n} + 1")
	if(n EQUAL 8)
		string(APPEND bin "${line}\n")
		set(line "")
		set(n 0)
	endif()
endforeach()
if(line)
	string(APPEND bin "${line}\n")
endif()

file(WRITE ${here}/psiddrv.bin "${bin}")
message(STATUS "psiddrv.bin written")
