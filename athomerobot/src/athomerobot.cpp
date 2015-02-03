#include "ros/ros.h"
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <math.h>
#include <sstream>
#include <string>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <tbb/atomic.h>

#include "serial/serial.h"
#include "std_msgs/Int32.h"
#include "std_msgs/String.h"
#include "std_msgs/Bool.h"
#include "geometry_msgs/Twist.h"
#include <sensor_msgs/LaserScan.h>

#include "athomerobot_msgs/arm.h"
#include "athomerobot_msgs/omnidata.h"
#include "athomerobot_msgs/head.h"
#include "athomerobot_msgs/irsensor.h"


#include <dynamixel_msgs/MotorStateList.h>
#include <dynamixel_msgs/JointState.h>

#include <dynamixel_controllers/SetComplianceSlope.h>
#include <dynamixel_controllers/SetCompliancePunch.h>

#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#define ENABLE_SERIAL
#define FILTER_QUEU 10
//=============================================================

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;

using namespace std;
using namespace boost;

void Omnidrive(float vx, float vy, float w);
bool robot_init = false;
bool App_exit = false;
int init_counter = 0;
int control_mode = 0;

int g_Motor[10] = {0};
int g_Motortemp[10] = {0};
int s_Motor[10] = {0};
int p_Motor[10] = {0};
float l_Motor[10] = {0};
int v_Motor[10] = {0};

float L = 50; //cm
int mobileplatform_motors_write[4] = {128, 128, 128, 128};
int mobileplatform_motors_read[4] = {128, 128, 128, 128};
tbb::atomic<int> keypad_status_raw;
tbb::atomic<int> keypad_status;
tbb::atomic<int> last_keypad_value;
int serial_state = 1;

float laser_IR[8];
int IR[5] = {0};
int EMS_STOP = 0;
ros::Publisher chatter_pub[20];
ros::Publisher chatter_pub_motor[20];
tbb::atomic<int> Compass;

int key_pad_reset = 0;
int robot_max_speedx = 300;
int robot_max_speedy = 300;
int robot_max_speedw = 250;

int inputload_right = 0; //gripper desire load right
int inputload_left = 0; //gripper desire load right
int voltage_down = 0;
int voltage_up = 0;

bool z_busy = false;
int z_reset_counter = 0;

bool green_light = false;
bool red_light = false;
bool btn_start = false;
bool btn_stop = false;

int serial_read_hz = 0;
int serial_rw_count = 0;

inline float Deg2Rad(float deg)
{
    return deg * M_PI / 180;
}

inline float Rad2Deg(float rad)
{
    return rad * 180 / M_PI;
}

void chatterCallback_loadgripper_right(const std_msgs::Int32::ConstPtr &msg)
{
    inputload_right = msg->data;

    if (inputload_right < -1) inputload_right = -1;
    if (inputload_right > 10) inputload_right = 10;
    inputload_right = inputload_right * 10;
}

void chatterCallback_loadgripper_left(const std_msgs::Int32::ConstPtr &msg)
{
    inputload_left = msg->data;

    if (inputload_left < -1) inputload_left = -1;
    if (inputload_left > 10) inputload_left = 10;
    inputload_left = inputload_left * 10;
}

void chatterCallback_head(const athomerobot_msgs::head::ConstPtr &msg)
{
    g_Motor[9] = msg->pan;
    g_Motor[7] = msg->tilt;
}

void chatterCallback_arm_right(const athomerobot_msgs::arm::ConstPtr &msg)
{
    g_Motor[0] = msg->shoulder_pitch;
    g_Motor[1] = msg->shoulder_roll;
    g_Motor[2] = msg->elbow;
    g_Motor[3] = msg->wrist_pitch;
    g_Motor[4] = msg->wrist_roll;
}

void chatterCallback_arm_left(const athomerobot_msgs::arm::ConstPtr &msg)
{
    // g_Motor[8] = msg->shoulder_pitch;
    // g_Motor[9] = msg->shoulder_roll;
    // g_Motor[10] = msg->elbow;
    // g_Motor[11] = msg->wrist_pitch;
    // g_Motor[12] = msg->wrist_roll;
}

int convert_desire(int z)
{
   int des = z - 320;
   des = (int)(((float)des * 200) / 100) + 520;
   return des;
}

void chatterCallback_desireZ(const std_msgs::Int32::ConstPtr &msg)
{
    //420 => 1024
    //320 => 512
    int des = msg->data - 320;
    int motor = (int)(((float)des * 200) / 100) + 520;
    g_Motor[8] = motor;
//
//    dynamixel_controllers::TorqueEnable srv;
//    srv.request.torque_enable = true;
//    service_call_torque[8].call(srv);

}

void chatterCallback_greenlight(const std_msgs::Bool::ConstPtr &msg)
{
    cout<<msg->data<<endl;
    green_light = msg->data;
}

void chatterCallback_redlight(const std_msgs::Bool::ConstPtr &msg)
{
    cout<<msg->data<<endl;
    red_light = msg->data;
}

void chatterCallback_laser(const sensor_msgs::LaserScan::ConstPtr &msg)
{

    int val_count = msg->ranges.size(); //512
    //cout<<val_count<<" "<<endl;

    float read = 0;
    int valid_count = 0;

    for ( int i = 0 ; i < 8 ; i++)
    {
        read = 0;
        valid_count = 0;
        laser_IR[7 - i] = 0;

        for ( int j = 0 ; j < 64 ; j++)
        {
            read = msg->ranges[i * 64 + j];

            if ( !std::isnan(read) && !isinf(read) )
            {
                laser_IR[7 - i] += read;
                valid_count++;
            }
        }

        if ( valid_count > 0)
            laser_IR[7 - i] = (laser_IR[7 - i] / valid_count) * 100;
        else
            laser_IR[7 - i] = 400; //all segment points are damaged ...

        laser_IR[7 - i] = (int)laser_IR[7 - i];



        //cout<<i<<" "<<valid_count<<endl;
    }


    // cout << Zsensor << "\t" <<endl;
    //<< Compass << "\t" << laser_IR[0] << "\t" << laser_IR[1] << "\t" << laser_IR[2] << "\t" << laser_IR[3] << "\t" << laser_IR[4] << "\t" << laser_IR[5] << "\t" << laser_IR[6] << "\t" << laser_IR[7] << "\t" << endl;
}

void chatterCallback_omnidrive(const athomerobot_msgs::omnidata::ConstPtr &msg)
{
    Omnidrive(msg->d0, msg->d1, msg->d2);
}

void logic()
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    cout << "SEPANTA CORE STARTED DONE 17 july" << endl;
    while (App_exit == false)
    {
        serial_rw_count = 0;
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        serial_read_hz =  serial_rw_count;
        
    }
}

float vx_array[50];
float vy_array[50];
float vz_array[50];

void motor_update()
{
    float sum_x = 0;
    float sum_y = 0;
    float sum_z = 0;

    for  ( int i = 0 ; i < 50 ; i++ )
    {
       
    }
}


void Omnidrive(float vx, float vy, float vw)
{

    vw = -vw / 100;

    //speed limits
    if ( vx > robot_max_speedx ) vx = robot_max_speedx;
    if ( vy > robot_max_speedy ) vy = robot_max_speedy;
    if ( vw > robot_max_speedw ) vw = robot_max_speedw;

    if ( vw < -robot_max_speedx ) vw = -robot_max_speedx;
    if ( vy < -robot_max_speedy ) vy = -robot_max_speedy;
    if ( vx < -robot_max_speedw ) vx = -robot_max_speedw;

    //FK
    float w1 = (vx - vy - vw * (L)) / 7.5;
    float w2 = (vx + vy + vw * (L)) / 7.5;
    float w3 = -(vx + vy - vw * (L)) / 7.5;
    float w4 = -(vx - vy + vw * (L)) / 7.5;

    w1 += 128;
    w2 += 128;
    w3 += 128;
    w4 += 128;

    if (w1 > 254) w1 = 254;
    if (w1 < 1) w1 = 1;
    if (w2 > 254) w2 = 254;
    if (w2 < 1) w2 = 1;
    if (w3 > 254) w3 = 254;
    if (w3 < 1) w3 = 1;
    if (w4 > 254) w4 = 254;
    if (w4 < 1) w4 = 1;

    //write to motors with cm9

    if ( EMS_STOP == 0) //check for emergency stop .... !
    {
        cout<<"send "<<w1<<" "<<w2<<" "<<w3<<" "<<w4<<endl;
        mobileplatform_motors_write[0] = w4;
        mobileplatform_motors_write[1] = w3;
        mobileplatform_motors_write[2] = w1;
        mobileplatform_motors_write[3] = w2;
    }
    else
    {
        cout<<"EMS STOP"<<endl;
        //emergency stop
        mobileplatform_motors_write[0] = 128;
        mobileplatform_motors_write[1] = 128;
        mobileplatform_motors_write[2] = 128;
        mobileplatform_motors_write[3] = 128;
    }
}

void control_rightgrip()
{

    while (App_exit == false)
    {
        if (inputload_right == -10 )
        {
            g_Motor[5] = 2048 + 350; //open lef gripper
            g_Motor[6] = 2048 - 350; //open lef gripper

            //cout << "-1" << endl;
        }
        else if (inputload_right == 0 )
        {
            g_Motor[5] = 2048 + 100; //open lef gripper
            g_Motor[6] = 2048 - 100; //open lef gripper

            //cout << "0" << endl;
        }
        else
        {
            while (abs(l_Motor[5]) < abs(inputload_right) && abs(l_Motor[6]) < abs(inputload_right))
            {
                g_Motor[5] -= 4;
                g_Motor[6] += 4;

                //  cout << l_Motor[5] << " " << l_Motor[6] << " " << endl;
                boost::this_thread::sleep(boost::posix_time::milliseconds(25));
            }

            //    cout << "end" << endl;
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }

}

void control_leftgrip()
{

    //         while (App_exit == false) {
    //             if (inputload_left == 0 ) {

    //                 g_Motor[13] = 512; //open lef gripper
    //             } else {

    //                 //close
    //                 while (l_Motor[13] < inputload_left) {
    //                     g_Motor[13] += 1;
    //                     boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    //                 }

    //                 //cout<<"close end"<<endl;
    //             }

    //             boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    //         }

}

void chatterCallbackw(const dynamixel_msgs::MotorStateList::ConstPtr &msg)
{
    for ( int i = 0 ; i < 10 ; i++ )
    {
        s_Motor[i] = msg->motor_states[i].speed;
        p_Motor[i] = msg->motor_states[i].position;
        l_Motor[i] = msg->motor_states[i].load * 1000;
        v_Motor[i] = msg->motor_states[i].voltage;
    }
}

int control_mode_old = 0;

void Motor_Update()
{
    std_msgs::Int32 msg[8];

    if ( init_counter < 20)
        init_counter++;
    else
        robot_init = true;

    for (int i = 0; i < 10; i++)
    {
        if ( g_Motortemp[i] != g_Motor[i] && robot_init == true)
        {

            g_Motortemp[i] = g_Motor[i];
            msg[i].data = g_Motor[i];

            for ( int m = 0 ; m < 10000000 ; m++ )
            {

            }

            chatter_pub_motor[i].publish(msg[i]);

            //cout << "motor id :" << i << "value" << g_Motor[i] << endl;
        }


    }


    //================================= Global Core Publishers

    std_msgs::Int32 msg_info;
    msg_info.data = Compass;
    chatter_pub[3].publish(msg_info);

    athomerobot_msgs::omnidata omni_info[2];
    omni_info[0].d0 = mobileplatform_motors_read[0];
    omni_info[0].d1 = mobileplatform_motors_read[1];
    omni_info[0].d2 = mobileplatform_motors_read[2];
    omni_info[0].d3 = mobileplatform_motors_read[3];

    chatter_pub[4].publish(omni_info[0]);

    athomerobot_msgs::irsensor sensor_info;
    sensor_info.d0 = IR[0];
    sensor_info.d1 = IR[1];
    sensor_info.d2 = IR[2];
    sensor_info.d3 = IR[3];
    sensor_info.d4 = IR[4];
    chatter_pub[6].publish(sensor_info);

    athomerobot_msgs::irsensor sensor_info_laser;
    sensor_info_laser.d0 = (int)laser_IR[0];
    sensor_info_laser.d1 = (int)laser_IR[1];
    sensor_info_laser.d2 = (int)laser_IR[2];
    sensor_info_laser.d3 = (int)laser_IR[3];
    sensor_info_laser.d4 = (int)laser_IR[4];
    sensor_info_laser.d5 = (int)laser_IR[5];
    sensor_info_laser.d6 = (int)laser_IR[6];
    sensor_info_laser.d7 = (int)laser_IR[7];
    chatter_pub[7].publish(sensor_info_laser);

    std_msgs::Int32 key_msg;
    key_msg.data = keypad_status;
    chatter_pub[8].publish(key_msg); //keypad

    std_msgs::Int32 ems_msg;
    ems_msg.data = EMS_STOP;
    chatter_pub[9].publish(ems_msg); //ems stop

    std_msgs::Int32 z_msg;
    z_msg.data = (p_Motor[8] - 520) * 100;
    z_msg.data = (int)((float)z_msg.data / 200) + 320;


    chatter_pub[2].publish(z_msg); //Z current pos



    std_msgs::Bool btn_msg;
    btn_msg.data = btn_start;
    chatter_pub[11].publish(btn_msg);// btn_start

    cout<<btn_start<<endl;

    std_msgs::Int32 mode_msg;
    mode_msg.data = control_mode;
    chatter_pub[12].publish(mode_msg); //Mode


    std_msgs::Int32 v1_msg;
    v1_msg.data = voltage_down;
    chatter_pub[13].publish(v1_msg); //voltage down

    std_msgs::Int32 v2_msg;
    v2_msg.data = voltage_up;
    chatter_pub[14].publish(v2_msg); //voltage up
    
    //cout<<control_mode<<" "<<control_mode_old<<endl;

   


}

void serial_logic()
{

#ifdef ENABLE_SERIAL
    cout << "serial reader CM9 started..." << endl;
    while (App_exit == false)
    {

        try
        {

            try
            {

                serial::Serial my_serial("/dev/serial/by-id/usb-ROBOTIS_CO._LTD._ROBOTIS_Virtual_COM_Port-if00", 1000000 , serial::Timeout::simpleTimeout(1000));

                cout << "Is the serial port open? ";
                if (my_serial.isOpen())
                {
                    cout << " Yes." << endl;
                }
                else
                    cout << " No." << endl;

                boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

                uint8_t result_write[30];
                uint8_t result_read[30];

                int queuercmpsx[FILTER_QUEU];
                int queuercmpsy[FILTER_QUEU];
                int queuerir1[FILTER_QUEU];
                int queuerir2[FILTER_QUEU];
                int queuerir3[FILTER_QUEU];
                int queuerir4[FILTER_QUEU];
                int queuerir5[FILTER_QUEU];
                int queuerv1[FILTER_QUEU];
                int queuerv2[FILTER_QUEU];

                serial_state = 1;

                while (App_exit == false)
                {
                    //serial loop
                    //cout<<"RW DONE!"<<endl;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(50));
                    serial_rw_count++;
                    /////////////////////////////////////////////////////
                    if ( serial_state == 1 ) //write
                    {

                        result_write[0] = 255;
                        result_write[1] = 190;
                        result_write[2] = 255;
                        result_write[3] = 100;
                        result_write[4] = 40;
                        result_write[5] = mobileplatform_motors_write[0];
                        result_write[6] = mobileplatform_motors_write[1];
                        result_write[7] = mobileplatform_motors_write[2];
                        result_write[8] = mobileplatform_motors_write[3];

                        char green = 25;
                        char red = 25;
                        if ( green_light ) green = 75;
                        if ( red_light   ) red = 75;

                        result_write[9] = red;
                        result_write[10] = green;

                        my_serial.write(result_write, 11);

                        boost::this_thread::sleep(boost::posix_time::milliseconds(10));

                        uint8_t read;

                        while (App_exit == false)
                        {
                            my_serial.read(&read, 1);

                            if ( read == 255 )
                            {
                                my_serial.read(&read, 1);

                                if ( read == 190 )
                                {
                                    my_serial.read(&read, 1);

                                    if ( read == 255)
                                    {
                                        // ROS_INFO("Header ok...");

                                        my_serial.read(result_read,21);

                                        mobileplatform_motors_read[0] = result_read[0];
                                        mobileplatform_motors_read[1] = result_read[1];
                                        mobileplatform_motors_read[2] = result_read[2];
                                        mobileplatform_motors_read[3] = result_read[3];

                                       // cout<<mobileplatform_motors_read[0]<<" "<<mobileplatform_motors_read[1]<<" "<<mobileplatform_motors_read[2]<<" "<<mobileplatform_motors_read[3]<<endl;

                                        if (  result_read[4] != 0)
                                        {
                                            keypad_status = result_read[4];
                                        }
                                       // cout<<keypad_status<<endl;

                                       int x = ( result_read[6] << 8 ) + ( result_read[5] ) ; //Compass (2 byte)
                                       int y = ( result_read[8] << 8 ) + ( result_read[7] ) ; //Compass (2 byte)
                                        
                                        control_mode = result_read[9];

                                        //cout<<"mode:"<<control_mode<<endl;

                                        for ( int i = FILTER_QUEU  - 2 ; i >= 0 ; i--)
                                        {
                                            queuercmpsx[i + 1] = queuercmpsx[i];
                                            queuercmpsy[i + 1] = queuercmpsy[i];
                                            queuerir1[i+1] =  queuerir1[i];
                                            queuerir2[i+1] =  queuerir2[i];
                                            queuerir3[i+1] =  queuerir3[i];
                                            queuerir4[i+1] =  queuerir4[i];
                                            queuerir5[i+1] =  queuerir5[i];
                                            queuerv1[i+1] = queuerv1[i];
                                            queuerv2[i+1] = queuerv2[i];

                                        }

                                        queuercmpsx[0] = x;
                                        queuercmpsy[0] = y;
                                        queuerir1[0] = (int)result_read[12];
                                        queuerir2[0] = (int)result_read[13];
                                        queuerir3[0] = (int)result_read[14];
                                        queuerir4[0] = (int)result_read[15];
                                        queuerir5[0] = (int)result_read[16];
                                        queuerv1[0] = ( result_read[18] << 8 ) + ( result_read[17] );
                                        queuerv2[0] = ( result_read[20] << 8 ) + ( result_read[19] );

                                        float sum1 = 0;
                                        float sum12 = 0;

                                        float sum2 = 0;
                                        float sum3 = 0;
                                        float sum4 = 0;
                                        float sum5 = 0;
                                        float sum6 = 0;
                                        float sum7 = 0;
                                        float sum8 = 0;

                                        for ( int i = 0 ; i < FILTER_QUEU ; i++ )
                                        {
                                            sum1 += queuercmpsx[i];
                                            sum12 += queuercmpsy[i];
                                            sum2 += queuerir1[i];
                                            sum3 += queuerir2[i];
                                            sum4 += queuerir3[i];
                                            sum5 += queuerir4[i];
                                            sum6 += queuerir5[i];
                                            sum7 += queuerv1[i];
                                            sum8 += queuerv2[i];
                                        }

                                        
                                        x = (int)(sum1 / FILTER_QUEU);
                                        y = (int)(sum12 / FILTER_QUEU);
                                        
                                        if ( x > 1000 )
                                            x = -1 * ( 65536 - x );

                                        if ( y > 1000 )
                                            y = -1 * ( 65536 - y );

                                        Compass = atan2(x,y) * 57.29;
                                        if (Compass < 0) Compass = Compass + 360;

                                       // cout<<"cmps :"<<Compass<<endl;
                                        //====================================

                                        char bstart = result_read[11];

                                        if ( bstart < 50 )
                                            btn_start = false;
                                        else
                                            btn_start = true;

                                        //cout<<btn_start<<endl;

                                        EMS_STOP = 0;

                                        IR[0] = (int)(sum2 / FILTER_QUEU);
                                        IR[1] = (int)(sum3 / FILTER_QUEU);
                                        IR[2] = (int)(sum4 / FILTER_QUEU);
                                        IR[3] = (int)(sum5 / FILTER_QUEU);
                                        IR[4] = (int)(sum6 / FILTER_QUEU);

                                        //cout<<IR[0]<<" "<<IR[1]<<" "<<IR[2]<<" "<<IR[3]<<" "<<IR[4]<<" "<<endl;

                                        voltage_up = (int)(sum7 / 10);
                                        voltage_down = (int)(sum8 / 10);

                                        //cout<< voltage_up <<" "<<voltage_down<<" "<<endl;
                                       // cout<<"Hz : "<<serial_read_hz<<endl;

                                            if ( control_mode_old != control_mode )
                                            {
                                            if ( control_mode == 1 )
                                            {
                                                red_light = true;
                                                control_mode_old = 1;
                                            }
                                            else
                                            {
                                                red_light = false;
                                                control_mode_old = 2;
                                            }
                                            }


                                        break;
                                    }
                                }
                            }


                        }

                    }


                }

            }//try
            catch (serial::SerialException e)
            {
                cout << "Read Error [CM9.04]" << endl;
            }

        }//try
        catch (serial::IOException e)
        {
            cout << "Port Not Opened [CM9.04]" << endl;
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    }//while
#endif
}

void init_motors()
{

    g_Motor[0] = 3000; //right_shoulder_pitch
    g_Motor[1] = 2048; //right_shoulder_roll
    g_Motor[2] = 2048; //right_elbow
    g_Motor[3] = 2048;  //right_wrist_pitch
    g_Motor[4] = 2048;  //right_wrist_roll
    g_Motor[5] = 2048; //right_gripper0
    g_Motor[6] = 2048; //right_gripper1
    g_Motor[7] = 1700;  //head tilt
    g_Motor[8] = convert_desire(350);  //Z MOTOR
    g_Motor[9] = 512;  //head pan
}

int main(int argc, char **argv)
{
    init_motors();
    Compass = 0;

    keypad_status_raw = 0;
    keypad_status = 0;
    last_keypad_value = 0;

    boost::thread _thread_logic(&logic);
    boost::thread _thread_serial(&serial_logic);
    boost::thread _thread_rightgrip(&control_rightgrip);
    boost::thread _thread_leftgrip(&control_leftgrip);

    ros::init(argc, argv, "athomerobot");

    ros::NodeHandle node_handles[50];
    ros::Subscriber sub_handles[15];

    //===========================================================================================
    chatter_pub[2] = node_handles[0].advertise<std_msgs::Int32>("AUTROBOTOUT_currentz", 10);
    chatter_pub[3] = node_handles[1].advertise<std_msgs::Int32>("AUTROBOTOUT_compass", 10);
    chatter_pub[4] = node_handles[2].advertise<athomerobot_msgs::omnidata>("AUTROBOTOUT_omnispeed", 10);
    chatter_pub[6] = node_handles[3].advertise<athomerobot_msgs::irsensor>("AUTROBOTOUT_irsensor", 10);
    chatter_pub[7] = node_handles[4].advertise<athomerobot_msgs::irsensor>("AUTROBOTOUT_lasersensor", 10);
    chatter_pub[8] = node_handles[5].advertise<std_msgs::Int32>("AUTROBOTOUT_keypad", 10);
    chatter_pub[9] = node_handles[6].advertise<std_msgs::Int32>("AUTROBOTOUT_EMSSTOP", 10);

    chatter_pub[11] = node_handles[35].advertise<std_msgs::Bool>("AUTROBOTOUT_btnstart", 10);
    chatter_pub[12] = node_handles[37].advertise<std_msgs::Int32>("AUTROBOTOUT_controlmode", 10); //1 down //2 up
    chatter_pub[13] = node_handles[38].advertise<std_msgs::Int32>("AUTROBOTOUT_voltagedown", 10);
    chatter_pub[14] = node_handles[39].advertise<std_msgs::Int32>("AUTROBOTOUT_voltageup", 10);
    //===========================================================================================
    sub_handles[0] = node_handles[7].subscribe("/motor_states/dx_port", 10, chatterCallbackw);
    sub_handles[1] = node_handles[8].subscribe("AUTROBOTIN_omnidrive", 10, chatterCallback_omnidrive);
    sub_handles[2] = node_handles[9].subscribe("scan", 10, chatterCallback_laser);
    sub_handles[3] = node_handles[10].subscribe("AUTROBOTIN_arm_right", 10, chatterCallback_arm_right);
    sub_handles[4] = node_handles[11].subscribe("AUTROBOTIN_desirez", 10, chatterCallback_desireZ);
    sub_handles[5] = node_handles[12].subscribe("AUTROBOTIN_gripper_right", 10, chatterCallback_loadgripper_right);
    sub_handles[6] = node_handles[13].subscribe("AUTROBOTIN_arm_left", 10, chatterCallback_arm_left);
    sub_handles[7] = node_handles[14].subscribe("AUTROBOTIN_gripper_left", 10, chatterCallback_loadgripper_left);
    sub_handles[8] = node_handles[15].subscribe("AUTROBOTIN_head", 10, chatterCallback_head);
    sub_handles[9] = node_handles[36].subscribe("AUTROBOTIN_greenlight", 10, chatterCallback_greenlight);
    sub_handles[10] = node_handles[37].subscribe("AUTROBOTIN_redlight", 10, chatterCallback_redlight);
    //============================================================================================
    chatter_pub_motor[0] = node_handles[16].advertise<std_msgs::Int32>("/rightshoulderPitch_controller/command", 10);
    chatter_pub_motor[1] = node_handles[17].advertise<std_msgs::Int32>("/rightshoulderRoll_controller/command", 10);
    chatter_pub_motor[2] = node_handles[18].advertise<std_msgs::Int32>("/rightelbow_controller/command", 10);
    chatter_pub_motor[3] = node_handles[19].advertise<std_msgs::Int32>("/rightwristRoll_controller/command", 10);
    chatter_pub_motor[4] = node_handles[20].advertise<std_msgs::Int32>("/rightwristPitch_controller/command", 10);
    chatter_pub_motor[5] = node_handles[21].advertise<std_msgs::Int32>("/rightgripper0_controller/command", 10);
    chatter_pub_motor[6] = node_handles[22].advertise<std_msgs::Int32>("/rightgripper1_controller/command", 10);
    chatter_pub_motor[7] = node_handles[23].advertise<std_msgs::Int32>("/headTilt_controller/command", 10);
    chatter_pub_motor[8] = node_handles[24].advertise<std_msgs::Int32>("/Zmotor_controller/command", 10);
    chatter_pub_motor[9] = node_handles[40].advertise<std_msgs::Int32>("/headPan_controller/command", 10);
    // chatter_pub_motor[9] = node_handles[25].advertise<std_msgs::Int32>("/leftshoulderRoll_controller/command", 10);
    // chatter_pub_motor[10] = node_handles[26].advertise<std_msgs::Int32>("/leftelbow_controller/command", 10);
    // chatter_pub_motor[11] = node_handles[27].advertise<std_msgs::Int32>("/leftwristPitch_controller/command", 10);
    // chatter_pub_motor[12] = node_handles[28].advertise<std_msgs::Int32>("/leftwristRoll_controller/command", 10);
    // chatter_pub_motor[13] = node_handles[29].advertise<std_msgs::Int32>("/leftgripper0_controller/command", 10);
    // chatter_pub_motor[14] = node_handles[30].advertise<std_msgs::Int32>("/headPitch_controller/command", 10);
    // chatter_pub_motor[15] = node_handles[31].advertise<std_msgs::Int32>("/headRoll_controller/command", 10);

    //=============================================================================================

 /*   service_call_torque[0] = node_handles[25].serviceClient<dynamixel_controllers::TorqueEnable>("/rightshoulderPitch_controller/torque_enable");
    service_call_torque[1] = node_handles[26].serviceClient<dynamixel_controllers::TorqueEnable>("/rightshoulderRoll_controller/torque_enable");
    service_call_torque[2] = node_handles[27].serviceClient<dynamixel_controllers::TorqueEnable>("/rightelbow_controller/torque_enable");
    service_call_torque[3] = node_handles[28].serviceClient<dynamixel_controllers::TorqueEnable>("/rightwristRoll_controller/torque_enable");
    service_call_torque[4] = node_handles[29].serviceClient<dynamixel_controllers::TorqueEnable>("/rightwristPitch_controller/torque_enable");
    service_call_torque[5] = node_handles[30].serviceClient<dynamixel_controllers::TorqueEnable>("/rightgripper0_controller/torque_enable");
    service_call_torque[6] = node_handles[31].serviceClient<dynamixel_controllers::TorqueEnable>("/rightgripper1_controller/torque_enable");
    service_call_torque[7] = node_handles[32].serviceClient<dynamixel_controllers::TorqueEnable>("/headTilt_controller/torque_enable");
    service_call_torque[8] = node_handles[33].serviceClient<dynamixel_controllers::TorqueEnable>("/Zmotor_controller/torque_enable");*/

    //================================================================================================
    ros::Rate loop_rate(15);

    while (ros::ok() && App_exit == false)
    {
        Motor_Update();
      //  Motor_Update_services();
        ros::spinOnce();
        loop_rate.sleep();
    }

    App_exit = true;

    _thread_leftgrip.interrupt();
    _thread_leftgrip.join();

    _thread_rightgrip.interrupt();
    _thread_rightgrip.join();

    _thread_logic.interrupt();
    _thread_logic.join();

    _thread_serial.interrupt();
    _thread_serial.join();

    return 0;
}
