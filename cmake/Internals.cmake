macro(fix_compiler_settings)
	if (CMAKE_C_COMPILER_ID MATCHES MSVC)
		message(STATUS "Setting C Compiler default warning level to /W4 and defining _CRT_SECURE_NO_WARNINGS")
		list(APPEND setting_list CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)

		foreach(setting ${setting_list})
			# Change default warning level in all C/CXX default compiler settings
			string(REGEX REPLACE "/W[0-4]" "/W4" ${setting} "${${setting}}")
			string(APPEND ${setting} " -D_CRT_SECURE_NO_WARNINGS")
		endforeach(setting)
	endif()

	if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
		message(STATUS "Setting CXX Compiler default warning level to /W4 and defining _CRT_SECURE_NO_WARNINGS")
		list(APPEND setting_list CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

		foreach(setting ${setting_list})
			# Change default warning level in all C/CXX default compiler settings
			string(REGEX REPLACE "/W[0-4]" "/W4" ${setting} "${${setting}}")
			string(APPEND ${setting} " -D_CRT_SECURE_NO_WARNINGS")
		endforeach(setting)
	endif()
endmacro()