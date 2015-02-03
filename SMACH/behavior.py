#!/usr/bin/env python

import roslib; roslib.load_manifest('turtlesim')
import rospy
import smach
import turtlesim
import math
import smach_ros 
from smach_ros import ServiceState
from turtlesim.srv import *

# define state BallPose
class BallPose(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['IsSeen','IsNotSeen'],
                                   input_keys=['isBallSeen'])


    def execute(self, userdata):
        rospy.loginfo('Executing state BallPose')
            
        if userdata.isBallSeen == 1 :
                return 'IsSeen'
        else:
                return 'IsNotSeen'

# define state Role
class Role(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['goalKeeper','attakerOrDefence'],
                                   input_keys=['myrole'])
    

    def execute(self, userdata):
        rospy.loginfo('Executing state Role')
        if userdata.myrole == 1 :
                return 'goalKeeper'
        else:
                return 'attakerOrDefence'

# define state SearchForBall
class SearchForBall(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['search'])


    def execute(self, userdata):
        rospy.loginfo('Executing state SearchForBall')
	return 'search'

# define state GoalKeeper
class GoalKeeper(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['ballShooted','getTeamatePose','attack'],
                                   input_keys=['ballX','ballY','isBallShooted','mapCenterX','mapCenterY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state GoalKeeper')
        if userdata.isBallShooted == 1:
            return 'ballShooted'

        if userdata.BallY > userdata.mapCenterY:
            return 'attack'
        else:
            return 'getTeamatePose'

# define state BallShooted
class BallShooted(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['sitDown'])


    def execute(self, userdata):
        rospy.loginfo('Executing state BallShooted')
	return 'sitDown'


# define state AttackMode
class AttackMode(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['stand'],
                                   input_keys=['goalLX','goalLY','goalRX','goalRY'],
                                   output_keys=['standX','standY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state AttackMode')
        userdata.standX=userdata.goalLX
        userdata.standY=(userdata.goalLY + userdata.goalRY)/2
	return 'stand'

# define state DefenciveMode
class DefenciveMode(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['haveDefence','dontHaveDefence'],
                                   input_keys=['ballX','ballY','teamateX','teamateY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state DefenciveMode')
        if userdata.BallY < userdata.teamateY:
            return 'dontHaveDefence'
        else:
            return 'haveDefence'

# define state DontHaveDefence
class DontHaveDefence(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['stand'],
                                   input_keys=['goalLX','goalLY','goalRX','goalRY'],
                                   output_keys=['standX','standY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state DontHaveDefence')
        userdata.standX=userdata.goalLX
        userdata.standY=(userdata.goalLY + userdata.goalRY)/2
	return 'stand'

# define state HaveDefence
class HaveDefence(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['ballisLeft','ballisRight'],
                                   input_keys=['ballX','ballY','centerX','centerY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state HaveDefence')
        if userdata.ballX >userdata.centerX:
            return 'ballisRight'
        else:
            return 'ballisLeft'

# define state BallisLeft
class BallisLeft(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['stand'],
                                   input_keys=['ballX','ballY','goalLX','goalLY','goalRX','goalRY'],
                                   output_keys=['standX','standY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state BallisLeft')
        userdata.standX=((userdata.goalLX + userdata.goalRX)/2+userdata.goalRX)/2
        userdata.standY=userdata.goalLY
	return 'stand'

# define state BallisRight
class BallisRight(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['stand'],
                                   input_keys=['ballX','ballY','goalLX','goalLY','goalRX','goalRY'],
                                   output_keys=['standX','standY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state BallisRight')
        userdata.standX=((userdata.goalLX + userdata.goalRX)/2+userdata.goalLX)/2
        userdata.standY=userdata.goalLY
        return 'stand'

# define state AttakerOrDefence
class AttakerOrDefence(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['haveBall','dontHaveBall'],
                                   input_keys=['ballX','ballY','x','y'])


    def execute(self, userdata):
        rospy.loginfo('Executing state AttakerOrDefence')
        if math.sqrt(math.pow((userdata.x-userdata.ballX),2)+ math.pow((userdata.y-userdata.ballY),2)) < 5:
                return 'haveBall'
        else:
                return 'dontHaveBall'

# define state HaveBall
class HaveBall(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['goodShootPose','badShootPose'],
                                   input_keys=['ballX','ballY','x','y','oppgoalKeeperX','oppgoalKeeperY','oppPlaceX','oppPlaceY','shootMaxDis'],
                                   output_keys=['shootAngle'])


    def execute(self, userdata):
        rospy.loginfo('Executing state HaveBall')
        userdata.shootAngle=0
        if math.sqrt(math.pow((userdata.x-userdata.ballX),2)+ math.pow((userdata.y-userdata.ballY),2)) > userdata.shootMaxDis:
                return 'badShootPose'
        if userdata.oppgoalKeeperX ==0 and userdata.oppgoalKeeperY ==0 and userdata.oppPlaceX==0 and userdata.oppPlaceY==0:
                return 'goodShootPose'
        if userdata.oppPlaceX == 0 and userdata.oppPlaceY ==0 and userdata.opponentgoalKeeperX !=0 and userdata.opponentgoalKeeperY !=0:
                return 'goodShootPose'
                

# define state GoodShootPose
class GoodShootPose(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['shoot'],
                                   input_keys=['shootPose'])


    def execute(self, userdata):
        rospy.loginfo('Executing state GoodShootPose')
	return 'shoot'

# define state BadShootPose
class BadShootPose(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['obstacle','noObstacle'],
                                   input_keys=['ballX','ballY','oppgoalLX','oppgoalLY','oppgoalRX','oppgoalRY','oppgoalKeeperX','oppgoalKeeperY','oppPlaceX','oppPlaceY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state BadShootPose')
        
        if userdata.oppPlaceY < userdata.ballY :
                return 'noObstacle'
        else:
                return 'obstacle'
                    

# define state NoObstacle
class NoObstacle(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['dribble'],
                                   input_keys=['ballX','ballY','oppgoalLX','oppgoalLY','oppgoalRX','oppgoalRY'],
                                   output_keys=['dribbleAng'])
                                             

    def execute(self, userdata):
        rospy.loginfo('Executing state NoObstacle')
        userdata.dribbleAng = 0
	return 'dribble'

# define state Obstacle
class Obstacle(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['isNear','isntNear'],
                                   input_keys=['x','y','oppgoalKeeperX','oppgoalKeeperY','oppPlaceX','oppPlaceY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state Obstacle')
        if userdata.BallX - userdata.oppPlaceX > 0 and userdata.BallX - userdata.oppPlaceX < 5 and userdata.oppPlaceY - userdata.BallY < 5:
                             return 'isNear'
        if userdata.oppPlaceX-userdata.BallX > 0 and userdata.oppPlaceX - userdata.BallX < 5 and userdata.oppPlaceY - userdata.BallY < 5:
                             return 'isNear'
        else:
                             return 'isntNear'

# define state IsNear
class IsNear(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['pass'],
                                   input_keys=['ballX','ballY','x','y','oppgoalKeeperX','oppgoalKeeperY','oppPlaceX','oppPlaceY','oppgoalLX','oppgoalLY','oppgoalRX','oppgoalRY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state IsNear')
        return 'pass'

# define state IsntNear
class IsntNear(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['pathPlannig'],
                                   input_keys=['ballX','ballY','oppgoalLX','oppgoalLY','oppgoalRX','oppgoalRY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state IsntNear')
	return 'pathPlannig'

# define state DontHaveBall
class DontHaveBall(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['opponentHasBall','ballIsFree'],
                                   input_keys=['ballX','ballY','oppgoalKeeperX','oppgoalKeeperY','oppPlaceX','oppPlaceY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state DontHaveBall')
        if math.sqrt(math.pow((userdata.oppPlaceX-userdata.ballX),2)+ math.pow((userdata.oppPlaceY-userdata.ballY),2)) < 5:
                    return 'opponentHasBall'
        if math.sqrt(math.pow((userdata.opponentgoalKeeperX-userdata.ballX),2)+ math.pow((userdata.opponentgoalKeeperY-userdata.ballY),2)) < 5:
                    return 'opponentHasBall'
        else:
                    return 'ballisFree'
                                             
# define state BallIsFree
class BallIsFree(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['goBehindBall'])


    def execute(self, userdata):
        rospy.loginfo('Executing state BallIsFree')
        return 'goBehindBall'

# define state GoBehindBall
class GoBehindBall(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['getBall'],
                                   input_keys=['ballX','ballY','x','y'],
                                   output_keys=['getBallPoseX','getBallPoseY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state GoBehindBall')
        userdata.getBallPoseX = 0
        if userdata.y - userdata.ballY > 0 :
            userdata.getBallPoseY = userdata.ballY-3
        else:
            userdata.getBallPoseY = userdata.ballY+3
	return 'getBall'

# define state OpponentHasBall
class OpponentHasBall(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['attacking','Defending'],
                                   input_keys=['ballX','ballY','x','y','oppgoalKeeperX','oppgoalKeeperY','oppPlaceX','oppPlaceY','goalLX','goalLY','goalRX','goalRY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state OpponentHasBall')
        return 'attacking'
                                             
# define state DefendRole
class DefendRole(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=['move'],
                                   input_keys=['ballX','ballY','x','y'],
                                   output_keys=['getBallPoseX','getBallPoseY'])


    def execute(self, userdata):
        rospy.loginfo('Executing state DefendRole')
        userdata.getBallPoseY=userdata.y
        if userdata.ballX > userdata.x :
                if userdata.x +4 < userdata.ballX:
                     userdata.getBallPoseX=  userdata.x + 4
                else:
                     userdata.getBallPoseX=  userdata.x
        else:
                if userdata.x -4 < userdata.ballX:
                    userdata.getBallPoseX=  userdata.x - 4
                else:
                    userdata.getBallPoseX=  userdata.x
        return 'move'



                                             
def main():
       rospy.init_node('collabBehavior')
   
       sm = smach.StateMachine(outcomes=['search','sitDown','standCenter','standLeft','standRight','shoot','Dribble','Pass','PathPlanning','moveVerticalyToBall','GetBall'])
       sm.userdata.meX = 0
       sm.userdata.meY = 0
       sm.userdata.MyRole = 0

       sm.userdata.teamateX = 0
       sm.userdata.teamateY = 0

       sm.userdata.BallX = 0
       sm.userdata.BallY = 0
       sm.userdata.IsBallSeen = 0
       sm.userdata.IsBallShooted = 0
       

       sm.userdata.MapCenterX = 0
       sm.userdata.MapCenterY = 0
       sm.userdata.goalLeftSideX = 0
       sm.userdata.goalLeftSideY = 0
       sm.userdata.goalRightSideX = 0
       sm.userdata.goalRightSideY = 0
      
       sm.userdata.oppgoalLeftSideX = 0
       sm.userdata.oppgoalLeftSideY = 0
       sm.userdata.oppgoalRightSideX = 0
       sm.userdata.oppgoalRightSideY = 0
       sm.userdata.OpponentgoalKeeperX = 0
       sm.userdata.OpponentgoalKeeperY = 0
       sm.userdata.OpponentPlaceX = 0
       sm.userdata.OpponentPlaceY = 0
       
       sm.userdata.standPoseX = 0
       sm.userdata.standPoseY = 0
       sm.userdata.shootMaxDis = 0
                                             
       sm.userdata.goodShootAngle = 0
       sm.userdata.goodDribbleAngle = 0
                                             
       sm.userdata.goodGetBallPoseX = 0
       sm.userdata.goodGetBallPoseY = 0
	

       with sm:
	   smach.StateMachine.add('BallPose', BallPose(),
				  transitions={ 'IsSeen':'Role',
                                'IsNotSeen':'SearchForBall'},
				   remapping={'isBallSeen':'IsBallSeen'})

	   smach.StateMachine.add('Role', Role(),
				  transitions={ 'goalKeeper':'GoalKeeper',
                                'attakerOrDefence':'AttakerOrDefence'},
				   remapping={'myrole':'MyRole'})

	   smach.StateMachine.add('SearchForBall', SearchForBall(),
				  transitions={ 'search':'search'})

	   smach.StateMachine.add('GoalKeeper', GoalKeeper(),
				  transitions={ 'ballShooted':'BallShooted',
                                'getTeamatePose':'GetTeamatePose',
                                'attack':'AttackMode'},
				   remapping={'ballX':'BallX',
                              'ballY':'BallY',
                              'isBallShooted':'IsBallShooted',
                              'mapCenterX':'MapCenterX',
                              'mapCenterY':'MapCenterY'})

	   smach.StateMachine.add('BallShooted', BallShooted(),
				  transitions={ 'sitDown':'sitDown'})	
 

	   smach.StateMachine.add('AttackMode', AttackMode(),
				  transitions={ 'stand':'standCenter'},
				   remapping={'goalLX':'goalLeftSideX',
                              'goalLY':'goalLeftSideY',
                              'goalRX':'goalRightSideX',
                              'goalRY':'goalRightSideY',
                              'standX':'standX',
                              'standY':'standY'})


           smach.StateMachine.add('GetTeamatePose',
                                   ServiceState('/spawn',
                                                 Spawn,
                                                 request_slots = ['x',
                                                                  'y']),
                                                transitions={'succeeded':'DefenciveMode',
                                                             'aborted':'DontHaveDefence',
                                                             'preempted':'DontHaveDefence'},
                                                remapping={'x':'teamateX',
                                                           'y':'teamateY'})
                              
	   smach.StateMachine.add('DefenciveMode', DefenciveMode(),
				  transitions={ 'haveDefence':'HaveDefence',
                              'dontHaveDefence':'DontHaveDefence'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'teamateX':'teamateX',
                              'teamateX':'teamateY'})

	   smach.StateMachine.add('DontHaveDefence', DontHaveDefence(),
				  transitions={ 'stand':'standCenter'},
				   remapping={'goalLX':'goalLeftSideX',
					          'goalLY':'goalLeftSideY',
					          'goalRX':'goalRightSideX',
					          'goalRY':'goalRightSideY',
                              'standX':'standPoseX',
                              'standY':'standPoseY'})


	   smach.StateMachine.add('HaveDefence', HaveDefence(),
				  transitions={ 'ballisLeft':'BallisLeft',
						'ballisRight':'BallisRight'},
				  remapping={ 'ballX':'BallX',
					          'ballY':'BallY',
                              'centerX':'MapCenterX',
                              'centerY':'MapCenterY'})

	   smach.StateMachine.add('BallisLeft', BallisLeft(),
				  transitions={ 'stand':'standLeft'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'goalLX':'goalLeftSideX',
                              'goalLY':'goalLeftSideY',
                              'goalRX':'goalRightSideX',
                              'goalRY':'goalRightSideY',
                              'standX':'standPoseX',
                              'standY':'standPoseY'})

	   smach.StateMachine.add('BallisRight', BallisRight(),
				  transitions={ 'stand':'standRight'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'goalLX':'goalLeftSideX',
                              'goalLY':'goalLeftSideY',
                              'goalRX':'goalRightSideX',
                              'goalRY':'goalRightSideY',
                              'standX':'standPoseX',
                              'standY':'standPoseY'})

	   smach.StateMachine.add('AttakerOrDefence', AttakerOrDefence(),
				  transitions={ 'haveBall':'HaveBall',
                             		        'dontHaveBall':'DontHaveBall'},
				  remapping={ 'ballX':'BallX',
                              		      'ballY':'BallY',
                              		      'x':'meX',
                            		      'y':'meY'})

	   smach.StateMachine.add('HaveBall', HaveBall(),
				  transitions={ 'goodShootPose':'GoodShootPose',
                                'badShootPose':'BadShootPose'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'x':'meX',
                              'y':'meY',
                              'oppgoalKeeperX':'OpponentgoalKeeperX',
                              'oppgoalKeeperY':'OpponentgoalKeeperY',
                              'oppPlaceX':'OpponentPlaceX',
                              'oppPlaceY':'OpponentPlaceY',
                              'shootAngle':'goodShootAngle',
                              'shootMaxDis':'shootMaxDis',
			      'shootAngle':'goodShootAngle'})

	   smach.StateMachine.add('GoodShootPose', GoodShootPose(),
				  transitions={ 'shoot':'shoot'},
				  remapping={ 'shootPose':'goodShootPose'})

	   smach.StateMachine.add('BadShootPose', BadShootPose(),
				  transitions={ 'obstacle':'Obstacle',
                                'noObstacle':'NoObstacle'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'oppgoalLX':'oppgoalLeftSideX',
                              'oppgoalLY':'oppgoalLeftSideY',
                              'oppgoalRX':'oppgoalRightSideX',
                              'oppgoalRY':'oppgoalRightSideY',
                              'oppgoalKeeperX':'OpponentgoalKeeperX',
                              'oppgoalKeeperY':'OpponentgoalKeeperY',
                              'oppPlaceX':'OpponentPlaceX',
                              'oppPlaceY':'OpponentPlaceY'})

	   smach.StateMachine.add('NoObstacle', NoObstacle(),
				  transitions={ 'dribble':'Dribble'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'oppgoalLX':'oppgoalLeftSideX',
                              'oppgoalLY':'oppgoalLeftSideY',
                              'oppgoalRX':'oppgoalRightSideX',
                              'oppgoalRY':'oppgoalRightSideY',
                              'dribbleAng':'goodDribbleAngle'})

	   smach.StateMachine.add('Obstacle', Obstacle(),
				  transitions={ 'isNear':'IsNear',
                              'isntNear':'IsntNear'},
				  remapping={ 'x':'meX',
                              'y':'meY',
                              'oppgoalKeeperX':'OpponentgoalKeeperX',
                              'oppgoalKeeperY':'OpponentgoalKeeperY',
                              'oppPlaceX':'OpponentPlaceX',
                              'oppPlaceY':'OpponentPlaceY'})

	   smach.StateMachine.add('IsNear', IsNear(),
				  transitions={ 'pass':'Pass'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'x':'meX',
                              'y':'meY',
                              'oppgoalKeeperX':'OpponentgoalKeeperX',
                              'oppgoalKeeperY':'OpponentgoalKeeperY',
                              'oppPlaceX':'OpponentPlaceX',
                              'oppPlaceY':'OpponentPlaceY',
                              'oppgoalLX':'oppgoalLeftSideX',
                              'oppgoalLY':'oppgoalLeftSideY',
                              'oppgoalRX':'oppgoalRightSideX',
                              'oppgoalRY':'oppgoalRightSideY'})

	   smach.StateMachine.add('IsntNear', IsntNear(),
				  transitions={ 'pathPlannig':'PathPlanning'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'oppgoalLX':'oppgoalLeftSideX',
                              'oppgoalLY':'oppgoalLeftSideY',
                              'oppgoalRX':'oppgoalRightSideX',
                              'oppgoalRY':'oppgoalRightSideY'})


	   smach.StateMachine.add('DontHaveBall', DontHaveBall(),
				  transitions={ 'opponentHasBall':'OpponentHasBall',
                                'ballIsFree':'BallIsFree'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'oppgoalKeeperX':'OpponentgoalKeeperX',
                              'oppgoalKeeperY':'OpponentgoalKeeperY',
                              'oppPlaceX':'OpponentPlaceX',
                              'oppPlaceY':'OpponentPlaceY'})

	   smach.StateMachine.add('BallIsFree', BallIsFree(),
				  transitions={ 'goBehindBall':'GoBehindBall'})

	   smach.StateMachine.add('GoBehindBall', GoBehindBall(),
				  transitions={ 'getBall':'GetBall'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'x':'meX',
                              'y':'meY',
                              'getBallPoseX':'goodGetBallPoseX',
                              'getBallPoseY':'goodGetBallPoseY'})

	   smach.StateMachine.add('OpponentHasBall', OpponentHasBall(),
				  transitions={ 'attacking':'GoBehindBall',
                                'Defending':'DefendRole'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'x':'meX',
                              'y':'meY',
                              'oppgoalKeeperX':'OpponentgoalKeeperX',
                              'oppgoalKeeperY':'OpponentgoalKeeperY',
                              'oppPlaceX':'OpponentPlaceX',
                              'oppPlaceY':'OpponentPlaceY',
                              'goalLX':'goalLeftSideX',
                              'goalLY':'goalLeftSideY',
                              'goalRX':'goalRightSideX',
                              'goalRY':'goalRightSideY'})

	   smach.StateMachine.add('DefendRole', DefendRole(),
				  transitions={ 'move':'moveVerticalyToBall'},
				  remapping={ 'ballX':'BallX',
                              'ballY':'BallY',
                              'x':'meX',
                              'y':'meY',
                              'getBallPoseX':'goodGetBallPoseX',
                              'getBallPoseY':'goodGetBallPoseY'})





	   sis = smach_ros.IntrospectionServer('Behavior', sm, '/SM_ROOT')
           sis.start()
    
   # Execute SMACH plan
           outcome = sm.execute()
   
           rospy.spin()
           sis.stop()
   
if __name__ == '__main__':
       main()
