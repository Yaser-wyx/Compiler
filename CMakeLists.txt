cmake_minimum_required(VERSION 3.10)
project(spr C)
project(temp C)

aux_source_directory(test testList)
add_executable(temp ${testList})

include_directories(include)
set(CMAKE_C_STANDARD 99)

aux_source_directory(vm srcList)
aux_source_directory(utils utilsList)
aux_source_directory(parser parserList)
aux_source_directory(cli cliList)
aux_source_directory(object objectList)
add_executable(spr ${srcList} ${utilsList} ${parserList} ${cliList} ${objectList})



