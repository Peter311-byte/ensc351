# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(led ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(led LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET led POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:led>"
     "~/Ensc351/public/myApps/led" 
  COMMENT "Copying ARM executable to public NFS directory")