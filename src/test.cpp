/*
 * test.cpp
 *
 *  Created on: 17.10.2016
 *      Author: michal
 */

#include <ros/ros.h>
#include <osm_planner/dijkstra.h>
#include <osm_planner/osm_parser.h>
#include <osm_planner/newTarget.h>
#include <std_msgs/Int32.h>
#include <std_srvs/Empty.h>

//todo
//1. zistit preco pada pri hladany cesty v nejakom pripade  (sad_janka_krala.osm)
//2. treba urcit pociatocny bod [0,0] - asi prva suradnica, ktoru precita.
//   Do gpsCallback treba pridat nejaku funkciu, ktora urci pociatocny bod z prvej suradnice a ulozi ju do globalnej premennej OsmParser
//3. zvazit, ci nebude treba pouzivat action server
//4. preplanovanie trajektorie a odstranenie uzlu cez, ktory sa neda ist
//5. prepocitat zemepisne suradnice na kartezke - zatial je tam len vzorec x = (lon2 - lon1) * 1000; lon1 je bod v 0

class OsmPlanner{
public:
    OsmPlanner(std::string file) :
            osm(file),
            dijkstra(osm.getGraphOfVertex()), targetID(100), sourceID(130)
    {

        //init ros topics and services
        ros::NodeHandle n;

        //subscribers
        replanning_sub = n.subscribe("replanning", 1, &OsmPlanner::changeTarget, this);
        gps_position_sub = n.subscribe("fix", 1, &OsmPlanner::gpsCallback, this);
        change_source_sub = n.subscribe("change_source", 1, &OsmPlanner::changeSource, this);

        //services
        replanning_service = n.advertiseService("replanning", &OsmPlanner::replanning, this);

        //color of points
        colorPosition.r = 1.0f;
        colorPosition.g = 1.0f;
        colorPosition.b = 0.0f;
        colorPosition.a = 1.0;

        colorTarget.r = 1.0f;
        colorTarget.g = 0.0f;
        colorTarget.b = 0.0f;
        colorTarget.a = 1.0;

        //draw paths network
        osm.publishPath();

        //****************TEST PLANNING PATH FROM PARAM**********************//
        //-------------------------------------------------------------------//

        //finding nereast OSM node
        double source_lat, source_lon;
        n.param<double>("target_lon", target_longitude, 1);
        n.param<double>("target_lat", target_latitude, 1);
        n.param<double>("source_lon", source_lon, 1);
        n.param<double>("source_lat", source_lat, 1);

        sourceID = osm.getNearestPoint(source_lat, source_lon);
        ROS_INFO("source ID %d", sourceID);
        osm.publishPoint(sourceID, colorPosition);
        sleep(1);
        targetID = osm.getNearestPoint(target_latitude, target_longitude);
        ROS_INFO("target ID %d", targetID);
        osm.publishPoint(targetID, colorTarget);
        sleep(1);
        osm.publishPoint(target_latitude, target_longitude, colorTarget);


        //planning and publish final path
        osm.publishPath(dijkstra.findShortestPath(sourceID, targetID), target_latitude, target_longitude);
        //-------------------------------------------------------------------//
    }
private:

    OsmParser osm;
    Dijkstra dijkstra;

    int sourceID;
    int targetID;

    double target_longitude;
    double target_latitude;

    visualization_msgs::Marker::_color_type colorPosition;
    visualization_msgs::Marker::_color_type colorTarget;

    /* Subscribers */
    ros::Subscriber replanning_sub;
    ros::Subscriber gps_position_sub;
    ros::Subscriber change_source_sub;

    /* Services */
    ros::ServiceServer replanning_service;

    //todo prerobit na vlastny service (request NavSatFix targetu)
    bool replanning(osm_planner::newTarget::Request &req, osm_planner::newTarget::Response &res){

       target_latitude = req.target.latitude;
        target_longitude = req.target.longitude;

        targetID = osm.getNearestPoint(target_latitude, target_longitude);
        osm.publishPath(dijkstra.findShortestPath(sourceID, targetID), target_latitude, target_longitude);

        //todo dorobit v pripade, ze sa nenaplanuje, tak res.success = false
        res.success = true;
        return true;
    }

    //len pre test, bude sa pouzivat service replanning
    void changeTarget(const std_msgs::Int32::ConstPtr& msg) {

        targetID = msg->data;
        ROS_ERROR("change target %d", targetID);

        osm.publishPath(dijkstra.getSolution(targetID));
        osm.publishPoint(targetID, colorTarget);

    }

    //len pre test, bude sa pouzivat gpsCallback
    void changeSource(const std_msgs::Int32::ConstPtr& msg) {

        sourceID = msg->data;

        ROS_ERROR("change source %d", sourceID);
        //todo tato funkcia sa bude vykonvat len docasne, potom sa bude vykonavat v service replanning
        osm.publishPath(dijkstra.findShortestPath(sourceID, targetID));
        osm.publishPoint(sourceID, colorPosition);

    }

    void gpsCallback(const sensor_msgs::NavSatFix::ConstPtr& msg) {


        double lat = msg->latitude;
        double lon = msg->longitude;

        osm.publishPoint(lat, lon, colorTarget);
        sourceID = osm.getNearestPoint(lat, lon);

        ROS_ERROR(" new source %d", sourceID);

        osm.publishPoint(sourceID, colorPosition);
    }
};


int main(int argc, char **argv) {

	ros::init(argc, argv, "test_osm");
	ros::NodeHandle n;

	std::string file = "skuska.osm";
	n.getParam("filepath", file);
    OsmPlanner osm_planner(file);

    ros::spin();

return 0;
}