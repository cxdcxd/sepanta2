cmake_minimum_required(VERSION 2.8.3)
project(athomerobot)


## Find catkin and any catkin packages
find_package(catkin REQUIRED COMPONENTS
  sensor_msgs
  cv_bridge
  roscpp
  rospy
  genmsg
  pcl_conversions
  pcl_ros
  tf
  std_msgs
  nav_msgs
  geometry_msgs
  image_transport
  message_generation
  actionlib
  actionlib_msgs 
  dynamixel_msgs
  athomerobot_msgs
)
find_package(Boost COMPONENTS system thread date_time REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(PCL 1.2 REQUIRED)
find_package(OpenCV 2.2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EIGEN REQUIRED eigen3)

## Declare ROS messages and services
#add_message_files(FILES arm.msg head.msg omnidata.msg irsensor.msg coordinate.msg motortorques.msg joint.msg user.msg users.msg)
#add_service_files(FILES command.srv maptools.srv windows.srv)
#add_action_files(FILES slamaction.action sepanta.action)

## Generate added messages and services
#generate_messages(
#      DEPENDENCIES
#      std_msgs
#      actionlib_msgs
#)

catkin_package(
      INCLUDE_DIRS include 
      CATKIN_DEPENDS message_runtime
)

include_directories(include ${catkin_INCLUDE_DIRS} ${EIGEN_INCLUDE_DIRS} ${SERIAL_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

#serial library for sepanta Hardware serial comunication
add_library(serial src/serial.cpp src/impl/unix.cpp)
target_link_libraries(serial rt )

#athomerobot is sepanta core system
add_executable(athomerobot src/athomerobot.cpp)
target_link_libraries(athomerobot ${catkin_LIBRARIES} ${Boost_LIBRARIES} serial)
add_dependencies(athomerobot athomerobot_msgs_generate_messages_cpp)

#compute sepanta odometry position and speed and publish /odom for rosnavigation 
#also this node publish the filtered data of laser as /laser_filter
add_executable(odometry src/odometry.cpp)
target_link_libraries(odometry ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES})
add_dependencies(odometry athomerobot_msgs_generate_messages_cpp)

#send slam target to Ros navigation system
add_executable(slam src/slam.cpp)
target_link_libraries(slam ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS})
add_dependencies(slam athomerobot_msgs_generate_messages_cpp)

add_executable(slam_client src/slam_client.cpp)
target_link_libraries(slam_client  ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS})
add_dependencies(slam_client athomerobot_msgs_generate_messages_cpp)

add_executable(communication src/communication.cpp)
target_link_libraries(communication  ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(communication athomerobot_msgs_generate_messages_cpp)

add_executable(tcpserver src/tcpserver.cpp)
target_link_libraries(tcpserver  ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(tcpserver athomerobot_msgs_generate_messages_cpp)

add_executable(tcpclient src/tcpclient.cpp)
target_link_libraries(tcpclient ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(tcpclient athomerobot_msgs_generate_messages_cpp)

add_executable(gesture src/gesture.cpp)
target_link_libraries(gesture ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(gesture athomerobot_msgs_generate_messages_cpp)

add_executable(xsense src/xsense.cpp)
target_link_libraries(xsense ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(xsense athomerobot_msgs_generate_messages_cpp)


add_executable(inspection src/inspection.cpp)
target_link_libraries(inspection ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(inspection athomerobot_msgs_generate_messages_cpp)

add_executable(hectoroffset src/hector_offset.cpp)
target_link_libraries(hectoroffset ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(hectoroffset athomerobot_msgs_generate_messages_cpp)

add_executable(egpsr src/egpsr.cpp)
target_link_libraries(egpsr ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(egpsr athomerobot_msgs_generate_messages_cpp)

add_executable(demo src/demo.cpp)
target_link_libraries(demo ${catkin_LIBRARIES} ${Boost_LIBRARIES} ${PCL_LIBRARIES} ${OpenCV_LIBS} serial)
add_dependencies(demo athomerobot_msgs_generate_messages_cpp)


