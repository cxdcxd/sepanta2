#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <tf/transform_broadcaster.h>
#include <stdio.h>

#include <actionlib/server/simple_action_server.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <athomerobot_msgs/slamactionAction.h>
#include <athomerobot_msgs/avoidthatactionAction.h>
#include <string>
#include <stdio.h>

#include "std_msgs/Int32.h"
#include "std_msgs/Bool.h"
#include "std_msgs/String.h"

#include "athomerobot_msgs/arm.h"
#include "athomerobot_msgs/head.h"
#include "athomerobot_msgs/omnidata.h"

#include "athomerobot_msgs/joint.h"
#include "athomerobot_msgs/user.h"
#include "athomerobot_msgs/users.h"

#include <athomerobot_msgs/maptools.h>
#include <athomerobot_msgs/follow.h>

#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "speak.h"

using namespace std;
using namespace boost;

vector <string> drinks;
vector <string> snacks;
vector <string> locations;

vector <string> drinks_orders;
vector <string> snacks_orders;
string location_first;
string location_second;

vector <string> fields;

ros::ServiceClient client;
ros::ServiceClient serviceclient_followme;

athomerobot_msgs::maptools mapsrv;
athomerobot_msgs::follow followsrv;

bool globalRuningFlag = true ;
int robot_state = -1;
int restaurant_state = -1;
int place_counter = 0;

float temp_person_error;
float front_person_error;

string speech_message = "";

int lastsceen_direction ; //-1 0 1

float desire_distance = 120; //cm
float desire_distance_tolerance = 10;
int vx_limit = 500;
int vw_limit = 500;

typedef actionlib::SimpleActionClient<athomerobot_msgs::slamactionAction> slam_Client;
slam_Client *globalClient_slam;

ros::Publisher _head;
ros::Publisher _gripper_right;
ros::Publisher _arm_right;
ros::Publisher _desired_z;
ros::Publisher _omni_drive;
ros::Publisher _speak;
ros::Publisher _greenlight;
ros::Publisher _redlight;

int person_x = 0;
int person_y = 0;
int person_z = 0;
int room_counter = 0;
int second = 0;
bool timer_reset = false;
bool timer_start = false;
bool flusher = false;
bool oneflush = false;
int min_index;
int min_z;

struct restaurant_point
{
public :
    int x;
    int y;
    int theta;
    string name;
};
std::vector<restaurant_point> restaurant_points;

void init()
{
    drinks.push_back("coca");
    drinks.push_back("milk");
    drinks.push_back("sunich");

    snacks.push_back("bread");
    snacks.push_back("pizza");
    snacks.push_back("pasta");

    locations.push_back("a");
    locations.push_back("b");
    locations.push_back("c");
}

void red_light(bool status)
{
    std_msgs::Bool msg;
    msg.data = status;
    _redlight.publish(msg);
}

void green_light(bool status)
{
    std_msgs::Bool msg;
    msg.data = status;
    _greenlight.publish(msg);
}

void save_point(string name)
{
    //add new user marked point with maptools to main slam map
    mapsrv.request.command = "res_savepoint";
    mapsrv.request.id = name;
    client.call(mapsrv);

    ROS_INFO("point saved");
}

void send_followme(string msg)
{
    followsrv.request.command = msg;
    serviceclient_followme.call(followsrv);
}

void save_pngmap()
{
    //we save the png map to see the points !
    
    mapsrv.request.command = "res_savemap";
    client.call(mapsrv);

    ROS_INFO("png_saved");
}


string speech_data = "";

void chatterCallback_speech(const std_msgs::String::ConstPtr &msg)
{
    speech_data = msg->data;

    //we got
    // if ( restaurant_state == 2 )
    // {
    //     //guide phase
    //     if ( msg->data != "" && speech_message != msg->data )
    //     {
    //         cout<<"speech : "<<msg->data<<endl;
    //         speech_message = msg->data;
    //         restaurant_state = 3;
    //     }
    // }

    // if ( restaurant_state == 5 )
    // {
    //    //move phase
    //     if ( msg->data != "" && speech_message != msg->data )
    //     {
    //         cout<<"speech : "<<msg->data<<endl;
    //         speech_message = msg->data;
    //         restaurant_state = 6;
    //     }

    // }
}

void gotolocation(string ID)
{
    actionlib::SimpleActionClient<athomerobot_msgs::slamactionAction> ac("slam_action", true);

    cout << "wait for slam server" << endl;
    globalClient_slam = &ac;
    ac.waitForServer();
    cout << "connect to slam server" << endl;
    athomerobot_msgs::slamactionGoal interfacegoal;
    interfacegoal.x = 0;
    interfacegoal.y = 0;
    interfacegoal.yaw = 0;
    interfacegoal.ID = ID;

    ac.sendGoal(interfacegoal);
    cout << "goal send" << endl;

    bool finished_before_timeout = ac.waitForResult();

    actionlib::SimpleClientGoalState state_ac_slam = ac.getState();

    if (state_ac_slam == actionlib::SimpleClientGoalState::SUCCEEDED)
    {
        ROS_INFO("SLAM DONE");
        athomerobot_msgs::slamactionResultConstPtr result_slam ;
        result_slam = ac.getResult();
        string result = result_slam->result;
        cout << result << endl;
    }
    else
    {
        ROS_INFO("SLAM ACTION FAILED...");
    }

}

void speak(string val)
{
    std_msgs::String msg;
    msg.data = val;
    _speak.publish(msg);
}

int main_state = 0;

void logicold()
{
    while( globalRuningFlag )
    {
        if ( restaurant_state == -3 )
        {
           //wait for speech
            cout<<"wait for speech"<<endl;
        }

        if ( restaurant_state == -2 )
        {
            cout<<"wait..."<<endl;
            if ( room_counter == 5)
            {
                send_followme("kill");
                cout<<"guid phase finished..."<<endl;
                restaurant_state = 4;
            }
        }

        if ( restaurant_state == -1 )
        {
            ROS_INFO("wait for person");
            green_light(false);
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            restaurant_state = -2;
        }

        if ( restaurant_state == 0 )
        {
             //follow the person...
             cout<<"wait for room/place "<<place_counter<<endl;
             restaurant_state = 1;
        }

        if ( restaurant_state == 1 )
        {
             //wait for follow
            //cout<<"wait for follow"<<endl;
        }

        if ( restaurant_state == 2 )
        {
            //wait for speech resault
            speak("res_start");
            green_light(true);
            place_counter++;
            restaurant_state = -2;
            cout<<"spead start"<<endl;
        }

        if ( restaurant_state == 3 )
        {
            //we got speech text name
            save_point(speech_message);
            restaurant_state = 0; //back to follow
            send_followme("continue");
            green_light(false);
            cout<<"countine"<<endl;
        }

        if ( restaurant_state == 4 )
        {
            //move phase
            cout<<"got to origin"<<endl;
            gotolocation("Origin");
            restaurant_state = 5;

        }

        if (restaurant_state == 5 )
        {
           //wait for speech
            speak("res_order");
            green_light(true);
            restaurant_state = -3;
            cout<<"res order"<<endl;

        }

        if (restaurant_state == 6 )
        {
           //wait for slam

            cout<<"state = 6 move phase started..."<<endl;

            boost::split( fields, speech_message, boost::is_any_of( "," ) );
            bool grip_done = false;

            for ( int i = 0 ; i < fields.size() ; i++)
            {
                if ( i % 2 == 0 )
                {
                    grip_done = false;
                    for ( int j = 0 ; j < drinks.size() ; j++)
                    {
                        if ( drinks[j] == fields[i])
                        {
                            gotolocation("drinks");
                            //=====================
                            //run grip for field[i]
                            break;
                        }
                    }

                    for ( int j = 0 ; j < snacks.size() ; j++)
                    {
                        if ( snacks[j] == fields[i])
                        {
                            gotolocation("snacks");
                            //=====================
                            //run grip for field[i]
                            break;
                        }
                    }


                }
                else
                {
                    if ( grip_done)
                    gotolocation(fields[i]);
                }
            }

            //all done
            restaurant_state = 7;

        }

        if ( restaurant_state == 7 )
        {
            cout<<"restaurant finish"<<endl;
            restaurant_state = 8;
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
}

void logic()
{
    
    string speechdataold = "";
    while( globalRuningFlag )
    {
        if ( main_state == 0 )
        {
           //wait for speech

           //cout<<"wait for speech global start"<<endl;

           if (  speech_data != "")
           {

              speak::say(speech_data.c_str());

              speech_data = "";

           }
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
    }
}

bool checkcommand(athomerobot_msgs::follow::Request  &req,athomerobot_msgs::follow::Response &res)
{
    std::string str = req.command;
    cout<<"restaurant service request..."<<endl;

    if ( str == "inroom")
    {
       restaurant_state = 2;
    }

    if ( str == "start")
    {
       restaurant_state = 0;
    }

    return true;
}

int main(int argc, char **argv)
{
    init();
    ros::init(argc, argv, "RESTAURANT_server");
    ROS_INFO("SEPANTA RESTAURANT TASK STARTED [DEMO] (93/11/03)...");

    ros::NodeHandle node_handle[15];
    ros::Subscriber node_subscribe[15];

    boost::thread _thread_logic(&logic);

    _desired_z = node_handle[0].advertise<std_msgs::Int32>("AUTROBOTIN_desirez", 10);
    _gripper_right = node_handle[1].advertise<std_msgs::Int32>("AUTROBOTIN_gripper_right", 10);
    _arm_right = node_handle[2].advertise<athomerobot_msgs::arm>("AUTROBOTIN_arm_right", 10);
    _head = node_handle[3].advertise<athomerobot_msgs::head>("/AUTROBOTIN_head", 10);
    _omni_drive = node_handle[5].advertise<athomerobot_msgs::omnidata>("/AUTROBOTIN_omnidrive", 10);
    _speak = node_handle[6].advertise<std_msgs::String>("/AUTROBOTOUT_speak", 10);

    _greenlight = node_handle[7].advertise<std_msgs::Bool>("/AUTROBOTIN_greenlight", 10);
    _redlight = node_handle[8].advertise<std_msgs::Bool>("/AUTROBOTIN_redlight", 10);

    //================================================================================

    //node_subscribe[0] = node_handle[4].subscribe("AUTROBOTOUT_skeleton", 10, chatterCallback_skeleton);
    node_subscribe[1] = node_handle[9].subscribe("AUTROBOTIN_speech", 10, chatterCallback_speech);

    ros::NodeHandle n_service;
    ros::ServiceServer service_command = n_service.advertiseService("RESTAURANT_in", checkcommand);

    ros::NodeHandle n_client2;
    serviceclient_followme = n_client2.serviceClient<athomerobot_msgs::follow>("FOLLOWME_in");

    ros::NodeHandle n_client;
    client = n_client.serviceClient<athomerobot_msgs::maptools>("AUTROBOTINSRV_maptools");

    ros::spin();

    _thread_logic.interrupt();
    _thread_logic.join();

    globalRuningFlag = false;

    return 0;
}
