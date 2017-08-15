//
// Created by michal on 31.12.2016.
/**
 * Inspired by http://www.geeksforgeeks.org/greedy-algorithms-set-6-dijkstras-shortest-path-algorithm/
 *
 * */
//

#ifndef OSM_LOCALIZATION_H
#define OSM_LOCALIZATION_H

#include <osm_planner/osm_parser.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>

#include <ros/ros.h>
#include <boost/thread.hpp>

#include <nav_msgs/Odometry.h>
#include <sensor_msgs/NavSatFix.h>


namespace osm_planner {

    class TfHandler{

    public:
        TfHandler(osm_planner::Parser::Haversine *calculator);
        void initThread();
        void setTfRotation(double angle);
        void improveTfPoseFromGPS(const sensor_msgs::NavSatFix::ConstPtr& gps);
        void improveTfRotation(double angle);

        geometry_msgs::Point getPoseFromTF(std::string map_link); //from tf

        void setFrames(std::string map_frame, std::string local_map_frame, std::string base_link_frame);
        std::string getMapFrame();
        std::string getBaseLinkFrame();
        std::string getLocalMapFrame();

    private:

        osm_planner::Parser::Haversine *calculator;

        std::string map_frame, base_link_frame, local_map_frame;

        /*tf broadcaster*/
        tf::TransformBroadcaster br;
        tf::Transform transform;

        double yaw;
        //tf broadcaster thread
        //    double initial_angle;
        boost::shared_ptr<boost::thread> tfThread;
        void tfBroadcaster();


        boost::mutex broadcaster_mutex;
    };

    class PathFollower{

    public:
        PathFollower(osm_planner::Parser *map);
        void addPoint(geometry_msgs::Point point);
        void doCorrection(TfHandler *tf);
        void setMaxDistance(double maxDistance);

    private:

        osm_planner::Parser *map;
        int firstNodeID;
        int currentNodeID;

        geometry_msgs::Point firstPosition;
        geometry_msgs::Point currentPosition;

        bool firstNodeAdded, secondNodeAdded;

        double angleDiff;

        double bearing;
        double maxDistance;

        void calculate();
        void clear();
    };


    class Localization {
    public:


        typedef struct point{
            int id;
            Parser::OSM_NODE geoPoint;
            geometry_msgs::PoseStamped cartesianPoint;
        }POINT;

        Localization(osm_planner::Parser *map);

        POINT *getCurrentPosition();
        TfHandler *getTF();

        double getFootwayWidth();
        bool isInitialized();


        void initialize();
        //todo prerobit lokalizacne veci z osm_planner sem

        //Before start make plan, this function must be call
        void initializePos(double lat, double lon, double bearing);
        void initializePos(double lat, double lon);
        void initializePos();

        //update pose
        bool setPositionFromGPS(const sensor_msgs::NavSatFix::ConstPtr& msg);        //from gps
        void setPositionFromOdom(geometry_msgs::Point point);  //from odom
        bool updatePoseFromTF();

        double checkDistance(int node_id, double lat, double lon);
        double checkDistance(int node_id, geometry_msgs::Pose pose);

    private:

        TfHandler tfHandler;
        PathFollower pathFollower;

        osm_planner::Parser *map;

        POINT source;

        bool initialized_ros;
        bool initialized_position;

        //global ros parameters
        bool matching_tf_with_map;
        int update_tf_pose_from_gps;
        double interpolation_max_distance;
        double footway_width;


        double getAccuracy(const sensor_msgs::NavSatFix::ConstPtr& gps);
    };

}

#endif //OSM_LOCALIZATION_H