#include <pcl/filters/voxel_grid.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>

#include "points_downsampler.h"

#define MAX_MEASUREMENT_RANGE 120.0

ros::Publisher filtered_points_pub;

// Leaf size of VoxelGrid filter.
static double voxel_leaf_size = 2.0;

static bool _output_log = false;
static std::ofstream ofs;
static std::string filename;

static std::string POINTS_TOPIC;

static void scan_callback(const sensor_msgs::PointCloud2::ConstPtr& input) {
  pcl::PointCloud<pcl::PointXYZ> scan;
  pcl::fromROSMsg(*input, scan);
  scan = removePointsByRange(scan, 0, MAX_MEASUREMENT_RANGE);

  pcl::PointCloud<pcl::PointXYZ>::Ptr scan_ptr(
      new pcl::PointCloud<pcl::PointXYZ>(scan));
  pcl::PointCloud<pcl::PointXYZ>::Ptr filtered_scan_ptr(
      new pcl::PointCloud<pcl::PointXYZ>());

  sensor_msgs::PointCloud2 filtered_msg;

  // if voxel_leaf_size < 0.1 voxel_grid_filter cannot down sample (It is
  // specification in PCL)
  if (voxel_leaf_size >= 0.1) {
    // Downsampling the velodyne scan using VoxelGrid filter
    pcl::VoxelGrid<pcl::PointXYZ> voxel_grid_filter;
    voxel_grid_filter.setLeafSize(voxel_leaf_size, voxel_leaf_size,
                                  voxel_leaf_size);
    voxel_grid_filter.setInputCloud(scan_ptr);
    voxel_grid_filter.filter(*filtered_scan_ptr);
    pcl::toROSMsg(*filtered_scan_ptr, filtered_msg);
  } else {
    pcl::toROSMsg(*scan_ptr, filtered_msg);
  }

  filtered_msg.header = input->header;
  filtered_points_pub.publish(filtered_msg);
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "voxel_grid_filter");

  ros::NodeHandle nh;
  ros::NodeHandle private_nh("~");

  private_nh.getParam("points_topic", POINTS_TOPIC);
  private_nh.getParam("output_log", _output_log);

  private_nh.param<double>("leaf_size", voxel_leaf_size, 2.0);
  ROS_INFO_STREAM("Voxel leaf size is: " << voxel_leaf_size);
  if (_output_log == true) {
    char buffer[80];
    std::time_t now = std::time(NULL);
    std::tm* pnow = std::localtime(&now);
    std::strftime(buffer, 80, "%Y%m%d_%H%M%S", pnow);
    filename = "voxel_grid_filter_" + std::string(buffer) + ".csv";
    ofs.open(filename.c_str(), std::ios::app);
  }

  // Publishers
  filtered_points_pub =
      nh.advertise<sensor_msgs::PointCloud2>("/filtered_points", 10);

  // Subscribers
  ros::Subscriber scan_sub = nh.subscribe(POINTS_TOPIC, 10, scan_callback);

  ros::spin();

  return 0;
}
