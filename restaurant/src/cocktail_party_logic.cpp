#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <athomerobot_msgs/slamactionAction.h>

#include "athomerobot_msgs/maptools.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <iostream>
#include <fstream>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <std_msgs/Int32.h>
#include <std_msgs/String.h>
#include <pcl/common/time.h>
#include <fstream>
#include "std_msgs/Bool.h"

/////////////TODO////////////////////
// #include <athomerobot_msgs/gesture_detectAction.h>
// #include <athomerobot_msgs/face_detectAction.h>
#include <athomerobot_msgs/grip_partyAction.h>

#include <athomerobot_msgs/sepantaAction.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <ros/package.h>
#include "athomerobot_msgs/arm.h"
#include "athomerobot_msgs/head.h"

//#include "emergency/position.h"
using namespace std;

#define FD_TIME 5 //TODO
#define GD_TIME 50 //TODO 
#define GF_TIME 10000 //TODO 

typedef actionlib::SimpleActionClient<athomerobot_msgs::slamactionAction> SLAMClient;
// typedef actionlib::SimpleActionClient<athomerobot_msgs::gesture_detectAction> GDClient;
// typedef actionlib::SimpleActionClient<athomerobot_msgs::face_detectAction> FDClient;
typedef actionlib::SimpleActionClient<athomerobot_msgs::grip_partyAction> GFClient;//TODO
typedef athomerobot_msgs::head head_msg;
typedef athomerobot_msgs::arm arm_msg_right;
typedef std_msgs::Int32 int_msg;

string  fetch_and_carry_action(string cm_obj);
void speechLogicCallback(const std_msgs::String &msg);
// void FindmeCallback(const std_msgs::String &msg);
void saveLocationToMap();
string location_person(string name_) ;
// void GestureDetection();
void goWithSlam(string where);
void logicThread();
// void facedetect();
// void tldLearn(int personCounter);
// void tldStop(int personCounter);
// void tldFind(int personnumforfinding);
void speak(string sentence);
void init();

SLAMClient *globalSLAM;
// GDClient *globalGD;
// FDClient *globalFD;
GFClient *globalGF;//TODO Bayad avaz she tebghe object
int id_global;
head_msg my_head_msg;
int_msg msg_z;

ros::Publisher FindmeLogicPublisher;
ros::Publisher speakPublisher;
ros::Publisher greenPublisher;
ros::Publisher n_head;
ros::Publisher arm_right;
ros::Publisher desired_z;

ros::ServiceClient client;
athomerobot_msgs::maptools srv;
string objectname[3];
string personname[3];
struct trip {
    string object;
    string person;
    string location;
};
string textComp = "Esi";
trip Orders_name[3];
string objectFetchedName = "";
string CarryObject;
int state = -1;
int personCounter = 0;
int deliveredObject = 0;
int personnumforfinding = 0;
int main(int argc, char **argv) {
    ros::init(argc, argv, "cocktail_party_logic");
    ros::NodeHandle speechNodeHandle;
    ros::NodeHandle advertiseNodeHandle;
    ros::NodeHandle LogicNodeHandleToTask;
    ros::NodeHandle n_client;


    ros::Subscriber speechSubscriber = speechNodeHandle.subscribe("AUTROBOTIN_speech", 1, speechLogicCallback);
    speakPublisher = advertiseNodeHandle.advertise<std_msgs::String>("/AUTROBOTOUT_speak", 10);/*for example AUTROBOT_from_find_me*/

    greenPublisher = advertiseNodeHandle.advertise<std_msgs::Bool>("AUTROBOTIN_greenlight", 10);
    // ros::Subscriber findmeLogicSubscriber = LogicNodeHandleToTask.subscribe("AUTROBOT_from_find_me_to_cocktail_logic", 1, FindmeCallback);
    // FindmeLogicPublisher = advertiseNodeHandle.advertise<std_msgs::String>("AUTROBOT_cocktail_logic_to_find_me", 1);

    client = n_client.serviceClient<athomerobot_msgs::maptools>("AUTROBOTINSRV_maptools");
    arm_right = advertiseNodeHandle.advertise<arm_msg_right>("AUTROBOTIN_arm_right", 10);
    n_head = advertiseNodeHandle.advertise<head_msg>("/AUTROBOTIN_head", 10);

    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));


    state = 3;
    boost::thread logic_thread(&logicThread);

    ros::spin();

    logic_thread.join();
    logic_thread.interrupt();


}

void logicThread() {
    // ros::Rate r(20);
    string obj_find;
    string l_person;
    string command_obj;
    while (ros::ok) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        if (state == 0) {
            init();
            state = 1;
        } else if (state == 1) { // go to center room
            ROS_INFO("state=1");
            goWithSlam("WDYS");
            state = 3;////////////state 2 hazv shood
        } else if (state == 2) { //wait for gesture detection
            // GestureDetection();///////  in state hazv shoode
        } else if (state == 3) { //gesture wasn't detect
            ROS_INFO("state=3");
             boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
             speak("come_near");
            // facedetect();
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
            state = 4;
        } else if (state == 4) { //near person ask the question
            ROS_INFO("state=4");

            cout << "personCounter   :=" << personCounter << endl;
             // saveLocationToMap();
              speak("ask_your_question");
            // Orders_name[0].object = "fix";
            // Orders_name[0].person = "ali";
            // Orders_name[1].object = "coca";
            // Orders_name[1].person = "ameneh";
            // Orders_name[2].object = "roozaneh";
            // Orders_name[2].person= "esi";

            // tldLearn(personCounter); //????
            state = -1;//state=-1;
        } else if (state == 5) {
            // tldStop(personCounter); ///???????
            ROS_INFO("state=5");
            ++personCounter;
            if ( personCounter < 3)

                state = 3; /// state 2 hazv shood
            else
               // state = 6;
            state = 7;
        } else if (state == 6) {
             speak("go_out");
            state=20;
            ROS_INFO("go_out");

        } else if (state == 7) {
            command_obj = "Grip_Init";
            obj_find = fetch_and_carry_action(command_obj);
            // goWithSlam("PPS");
            command_obj = "Grip_Start1";
            obj_find = fetch_and_carry_action(command_obj); //TO DO esme object hayi ke avarde shode hazf shavad
            cout<<"obj_find.....*******......"<<obj_find<<endl;
            if (obj_find == "") {
                cout<<"obj_find.....####......"<<obj_find<<endl;
                cout<<"salam"<<endl;
                state = 6;//state=6;
            }else{
                 cout<<"salam2"<<endl;
                state = 8;
            }
            //state=8
        } else if (state == 8) {
            l_person = location_person(obj_find);
            cout<<"obj_find......... "<<obj_find<<endl;
            // if (l_person == "") {
            //     state = 9;//state=6
            // } else {
                // goWithSlam(l_person);
                cout<<"inja1"<<endl;
                string id_person;          // string which will contain the result
                ostringstream convert_id;   // stream used for the conversion
                convert_id << id_global;      // insert the textual representation of 'Number' in the characters in the stream
                id_person = convert_id.str();
                 speak(id_person);
                state=9; ///?????????
             // }

        } else if (state == 9) {
            command_obj = "Grip_Start2";
            obj_find = fetch_and_carry_action(command_obj);
            state = 10;

        } else if (state == 10) { //person is standing in his place
            //ask about how deliver object and do that //TODO
            ++deliveredObject;
            if (deliveredObject < 3)
                state = 7;
            else
                state = 20;
        } else if (state == 11) { //person is not there
            //TODO   GestureDetection(); inja ham mitavan gesture tashkhis dad age are ki by by mikone bayad yeki in karo bokone
            //TODO Find TLD
            // tldFind(personnumforfinding);
        } else if (state == 20) {
            
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            goWithSlam("EA");
            init();
            ros::shutdown();
            break;
        }

    }

}

void init() {

    my_head_msg.tilt = 512; //0 ta 90
    n_head.publish(my_head_msg);

    arm_msg_right my_arm_msg;
    my_arm_msg.shoulder_pitch = 3000;
    my_arm_msg.shoulder_roll = 2048;
    my_arm_msg.elbow = 2048;
    my_arm_msg.wrist_pitch = 2048;
    my_arm_msg.wrist_roll = 2048;

    arm_right.publish(my_arm_msg);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    msg_z.data = 390;//beyne 2000 ta 3000 vali 2000 balast
    desired_z.publish(msg_z);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

}
// void FindmeCallback(const std_msgs::String &msg) {
//     ROS_INFO("findmelogicCallBack: msg: %s", msg.data.c_str());
//     if (msg.data == "person_not_found") {

//     } else if (msg.data == "find_person") {

//     }
// }
string location_person(string name_) {
    string obj_destination = "" ;

    for (int h = 0; h < 3; h++) {

        if (name_ == Orders_name[h].object) {
            // string id_location;          // string which will contain the result
            // ostringstream convert_id;   // stream used for the conversion
            // convert_id <<"$"<<Orders_name[h].location;      // insert the textual representation of 'Number' in the characters in the stream
            // id_location= convert_id.str();
            // obj_destination = id_location;
            Orders_name[h].object = "";
            id_global = h ;
            cout << "location **********" << obj_destination << endl;

        }
    }
    return obj_destination;

}
void goWithSlam(string where) {
    //edwin's code :D
    int R_TIME = 10000;
    SLAMClient slamAction("slam_action", true);
    globalSLAM = &slamAction ;
    ROS_INFO("wait for slam server");
    slamAction.waitForServer();
    ROS_INFO("connected to slam server");
    ROS_INFO("Going %s with slam...", where.c_str());
    athomerobot_msgs::slamactionGoal interfacegoal;
    interfacegoal.x = 0;
    interfacegoal.y = 0;
    interfacegoal.yaw = 0;
    interfacegoal.ID = where;

    //slamAction->sendGoal(interfacegoal);
    ROS_INFO("goal sent to slam... waiting for reach there.");

    slamAction.sendGoal(interfacegoal);
    bool finished_before_timeout = slamAction.waitForResult(ros::Duration(R_TIME));
    actionlib::SimpleClientGoalState state = slamAction.getState();
    if (state == actionlib::SimpleClientGoalState::SUCCEEDED) {
        ROS_INFO("Slam finished\n");
    } else {
        ROS_INFO("Finished Slam with current State: %s\n",
                 slamAction.getState().toString().c_str());
    }
}
void speechLogicCallback(const std_msgs::String &msg) {
    // std::cout << msg.data << std::endl;
    std::size_t againStr = (msg.data).find(textComp);
    std::size_t foundObjectNum0 = (msg.data).find("ObjectName0:");
    std::size_t foundObjectNum1 = (msg.data).find("ObjectName1:");
    std::size_t foundObjectNum2 = (msg.data).find("ObjectName2:");
    std::size_t PersonName0 = (msg.data).find("PersonName0:");
    std::size_t PersonName1 = (msg.data).find("PersonName1:");
    std::size_t PersonName2 = (msg.data).find("PersonName2:");

    if (msg.data == "ready") {
        std_msgs::Bool b_msg;
        b_msg.data = true;
        greenPublisher.publish(b_msg);
    } else if (msg.data == "stop") {
        std_msgs::Bool b_msg;
        b_msg.data = false;
        greenPublisher.publish(b_msg);
    }

    else if ((msg.data == "speech_done_finish")  && !(againStr != std::string::npos)) {
        state = 20;
        textComp = msg.data;
    } else if ((msg.data == "speech_done") && !(againStr != std::string::npos)) {
        state = 9;
        textComp = msg.data;
    }

    if ((foundObjectNum0 != std::string::npos)  && !(againStr != std::string::npos)) {
        Orders_name[0].object = msg.data;
        Orders_name[0].object.erase(0, 12);
        textComp = msg.data;

        cout << "objectname 0 : " << Orders_name[0].object << endl;
    } else if ((foundObjectNum1 != std::string::npos) && !(againStr != std::string::npos)) {
        Orders_name[1].object = msg.data;
        Orders_name[1].object.erase(0, 12);
        textComp = msg.data;

        cout << "objectname 1 : " << Orders_name[1].object << endl;
    } else if ((foundObjectNum2 != std::string::npos) && !(againStr != std::string::npos)) {
        Orders_name[2].object = msg.data;
        Orders_name[2].object.erase(0, 12);
        textComp = msg.data;

        cout << "objectname 2 : " << Orders_name[2].object << endl;
    } else if ((PersonName0 != std::string::npos)  && !(againStr != std::string::npos)) {
        Orders_name[0].person = msg.data;
        Orders_name[0].person.erase(0, 12);
        textComp = msg.data;

        cout << "personname 0 : " << Orders_name[0].person << endl;
    } else if ((PersonName1 != std::string::npos) && !(againStr != std::string::npos)) {
        Orders_name[1].person = msg.data;
        Orders_name[1].person.erase(0, 12);
        textComp = msg.data;

        cout << "personname 1 : " << Orders_name[1].person << endl;
    } else if ((PersonName2 != std::string::npos)  && !(againStr != std::string::npos)) {
        Orders_name[2].person = msg.data;
        Orders_name[2].person.erase(0, 12);
        textComp = msg.data;

        cout << "personname 2 : " << Orders_name[2].person << endl;
    } else if ((msg.data == "end") && !(againStr != std::string::npos)) {
        state = 5;
        ROS_INFO("state=5");
        textComp = msg.data;
    }
}

/*void GestureDetection() {

    GDClient GDAction("gesture_detect_action", true);
    globalGD = &GDAction;
    ROS_INFO("Waiting for GestureDetection action server to start.");

    GDAction.waitForServer();
    ROS_INFO("GestureDetection Action server started, sending goal.");
    athomerobot_msgs::gesture_detectGoal GDGoal;

    GDGoal.input = "start";
    GDAction.sendGoal(GDGoal);
    //wait for the action to return
    ROS_INFO("Waiting for Pick and Place to be dONE!");
    bool finished_before_timeout = GDAction.waitForResult(ros::Duration(GD_TIME));
    if (finished_before_timeout) {
        actionlib::SimpleClientGoalState stateGD = GDAction.getState();
        if (stateGD == actionlib::SimpleClientGoalState::SUCCEEDED) {
            state = 4;
            ROS_INFO("GD Action finished\n");
        } else {
            state = 3;
            ROS_INFO("Finished GD with current State: %s\n",
                     GDAction.getState().toString().c_str());
        }
    } else {
        ROS_INFO("GD did not finish before the time out.");

        GDAction.cancelGoal();
        GDAction.stopTrackingGoal();
        state = 3;
    }
}*/
/*void facedetect() {
    // ros::Rate loop_rate(20);
    FDClient FDAction("face_detect_action", true);
    globalFD = &FDAction;
    ROS_INFO("Waiting for GestureDetection action server to start.");

    FDAction.waitForServer();
    ROS_INFO("GestureDetection Action server started, sending goal.");
    athomerobot_msgs::face_detectGoal FDGoal;

    FDGoal.input = "start";
    FDAction.sendGoal(FDGoal);
    ROS_INFO("Waiting for Pick and Place to be dONE!");

    bool finished_before_timeout = FDAction.waitForResult(ros::Duration(FD_TIME));
    if (finished_before_timeout) {
        actionlib::SimpleClientGoalState stateFD = FDAction.getState();
        if (stateFD == actionlib::SimpleClientGoalState::SUCCEEDED) {
            if (state == 3)
                state = 4;
            else if (state == 7)
                state = 8;//taraf sare jash bud
            ROS_INFO("FD Action finished\n");
        } else {
            if (state == 3)
                state = 4;
            else if (state == 7)
                state = 9;

            ROS_INFO("Finished FD with current State: %s\n",
                     FDAction.getState().toString().c_str());
        }
    } else {
        ROS_INFO("FD did not finish before the time out.");

        FDAction.cancelGoal();
        FDAction.stopTrackingGoal();
        if (state == 3)
            state = 4;
        else if (state == 7)
            state = 9;

    }
}*/
void speak(string sentence) {
    std_msgs::String msg;
    msg.data = sentence;
    speakPublisher.publish(msg);
}

void saveLocationToMap() {
    srv.request.command = "savemarkedpoint";
    string PCS;          // string which will contain the result
    ostringstream convert;   // stream used for the conversion
    convert << personCounter;      // insert the textual representation of 'Number' in the characters in the stream
    PCS = convert.str(); // set 'Result' to the contents of the stream
    srv.request.id = "person " + PCS;
    cout << "location  = " << srv.request.id << endl;
    Orders_name[personCounter].location = srv.request.id ;
    client.call(srv);
}
string fetch_and_carry_action(string cm_obj) {

    GFClient GFAction("Grip_Party_action", true);

    globalGF = &GFAction;
    ROS_INFO("Waiting for GripAndFetch action server to start.");
    GFAction.waitForServer();
    ROS_INFO("pickAndPlace Action server started, sending goal.");
    athomerobot_msgs::grip_partyGoal GFGoal;


    GFGoal.object1 = Orders_name[0].object;
    GFGoal.object2 = Orders_name[1].object;
    GFGoal.object3 = Orders_name[2].object;
    GFGoal.input = cm_obj;

    GFAction.sendGoal(GFGoal);

    ROS_INFO("Waiting for Pick and Place to be dONE!");
    bool finished_before_timeout = GFAction.waitForResult(ros::Duration(GF_TIME));
    if (finished_before_timeout) {
        actionlib::SimpleClientGoalState stateGF = GFAction.getState();
        if (stateGF == actionlib::SimpleClientGoalState::SUCCEEDED) {

            ROS_INFO("GF Action finished\n");
            athomerobot_msgs::grip_partyResultConstPtr objectFetchName;
            objectFetchName = GFAction.getResult();
            objectFetchedName = objectFetchName->object_find;
        } else {
            ROS_INFO("Finished GF with current State: %s\n",
                     GFAction.getState().toString().c_str());
        }
    } else {
        ROS_INFO("GF did not finish before the time out.");

        GFAction.cancelGoal();
        GFAction.stopTrackingGoal();
    }
    return objectFetchedName;
}
/*void tldLearn(int personCounter) {
    std_msgs::String msg;
    string PCS;          // string which will contain the result
    ostringstream convert;   // stream used for the conversion
    convert << personCounter;      // insert the textual representation of 'Number' in the characters in the stream
    PCS = convert.str(); // set 'Result' to the contents of the stream
    msg.data = PCS;
    FindmeLogicPublisher.publish(msg);
    msg.data = "start_learn";
    FindmeLogicPublisher.publish(msg);
}
void tldStop(int personCounter ) {
    std_msgs::String msg;
    msg.data = "stop_learn";
    FindmeLogicPublisher.publish(msg);
}
void tldFind(int personnumforfinding) {
    int num;
    for (int i = 0 ; i < 3; ++i) {
        if (objectFetchedName == objectname[i])
            num = i;
    }
    std_msgs::String msg;
    string PCS;          // string which will contain the result
    ostringstream convert;   // stream used for the conversion
    convert << num;      // insert the textual representation of 'Number' in the characters in the stream
    PCS = convert.str(); // set 'Result' to the contents of the stream
    msg.data = PCS;
    FindmeLogicPublisher.publish(msg);
    msg.data = "find_person";
    FindmeLogicPublisher.publish(msg);
}*/
