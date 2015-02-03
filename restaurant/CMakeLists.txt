cmake_minimum_required(VERSION 2.8.3)
project(restaurant)
## Find catkin and any catkin packages
find_package(catkin REQUIRED COMPONENTS roscpp rospy std_msgs genmsg tf actionlib actionlib_msgs geometry_msgs nav_msgs athomerobot_msgs)
find_package(Boost COMPONENTS system thread date_time REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(PCL 1.2 REQUIRED)
pkg_check_modules(EIGEN REQUIRED eigen3)

## Declare ROS messages and services
#add_action_files(FILES restaurantaction.action)
## Generate added messages and services
#generate_messages(DEPENDENCIES std_msgs actionlib_msgs)

catkin_package()

include_directories(include ${catkin_INCLUDE_DIRS} ${EIGEN_INCLUDE_DIRS} )

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

#restaurant is sepanta core system
add_executable(restaurant src/restaurant.cpp)
target_link_libraries(restaurant ${catkin_LIBRARIES} ${Boost_LIBRARIES} )
add_dependencies(restaurant athomerobot_msgs_generate_messages_cpp)

