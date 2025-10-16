# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(assign1 ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(assign1 LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET assign1 POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:assign1>"
     "~/Ensc351/public/myApps/assign1" 
  COMMENT "Copying ARM executable to public NFS directory")