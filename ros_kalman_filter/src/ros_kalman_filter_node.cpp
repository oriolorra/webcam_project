#include "ros_kalman_filter_node.h"

using namespace Eigen;

//SET KALMAN PARAMETERS
// state vectors
Eigen::Vector4d x_before;
Eigen::Vector4d x_predicted;
Eigen::Vector4d x_t;

//noise state added
Eigen::Matrix4d C_nx;
double sigma_p_x = 10^2;
double sigma_v_x = 5^2;

//variance of the state
Eigen::Matrix4d C_x;
Eigen::Matrix4d C_x_predicted;
Eigen::Matrix4d C_x_before;

//z_t = H*x_t + n_z
Eigen::MatrixXd H(2,4);
Eigen::MatrixXd H_inv(2,4);
Eigen::MatrixXd H_T(4,2);

//Measurements vectors
Eigen::Vector2d z_t;
Eigen::Vector2d z_predicted;
//noise measurements added
Eigen::Matrix2d C_nz;

double sigma_nz;

//x_t = F*x_before + nx
Eigen::Matrix4d F;
double dT;
double precTick;
double ticks;

Eigen::MatrixXd K(2,4);
Eigen::Matrix4d I;

Eigen::Vector2d distance;

double dist;


RosKalmanFilterNode::RosKalmanFilterNode():
    nh_(ros::this_node::getName())
{
    std::cout << "////Ping 2a" << std::endl;
    //loop rate [hz], Could be set from a yaml file
    rate_=10;

    std::cout << "////Ping 2b" << std::endl;
    //set publishers
    std::cout << "////Ping 2c" << std::endl;

    kalman_msg_.layout.dim.resize(1);
    kalman_msg_.layout.dim[0].label = "kalman_states";
    kalman_msg_.layout.dim[0].size = 4;
    kalman_msg_.data.resize(4);
    kalman_publi = nh_.advertise<std_msgs::Float32MultiArray>("kalman_out", 100);

    std::cout << "////Ping 2d" << std::endl;
    //set subscribers
    detected_pixels = nh_.subscribe("/ros_face_detector/detector_out", 1, &RosKalmanFilterNode::centerFacePixelsCallbacks, this);

    std::cout << "////Ping 2e" << std::endl;

    x_predicted(0) = 320;
    x_predicted(1) = 240;
    x_predicted(2) = 0;
    x_predicted(3) = 0;

    C_nx << sigma_p_x, 0, 0, 0,
            0, sigma_p_x, 0, 0,
            0, 0, sigma_v_x, 0,
            0, 0, 0, sigma_v_x;

    C_x_before << 100, 0, 0, 0,
            0, 10^2, 0, 0,
            0, 0, 5^2, 0,
            0, 0, 0, 5^2;

    H << 1, 0, 0, 0,
         0, 1, 0, 0;

    sigma_nz = 20^2;
    C_nz << sigma_nz, 0,
            0, sigma_nz;

    dT = 0;
    precTick = 0;
    ticks = 0;

    F << 1, 0, dT, 0,
         0, 1, 0, dT,
         0, 0, 1, 0,
         0, 0, 0, 1;

    I << 1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0 ,1;

    std::cout << "////Ping 2f" << std::endl;
}

RosKalmanFilterNode::~RosKalmanFilterNode()
{
    // Destructor
}

void RosKalmanFilterNode::prediction()
{

    x_before = x_t;
    std::cout << "////Ping 4a" << std::endl;
    C_x_before = C_x;
    x_predicted = F * x_before;

    std::cout << "////Ping 4b" << std::endl;
    precTick = ticks;
    ticks = (double) cv::getTickCount();
    dT = (ticks - precTick) / cv::getTickFrequency(); //seconds

    C_x_predicted = F * C_x_before * F.transpose() + C_nx;
    std::cout << "////Ping 4c" << std::endl;
}

void RosKalmanFilterNode::correction()
{
    H_T = H.transpose();
    std::cout << "////Ping 5a:" << std::endl;
    z_predicted = H * x_predicted;
    std::cout << "////Ping 5b" << std::endl;

    std::cout << "////Ping 5c" << std::endl;


   // if(distanceMalanovich() <= 7){

        K = C_x_predicted * H_T * ((H * C_x_predicted * H_T) + C_nz).inverse();
        x_t = x_predicted + K*(z_t - z_predicted);

        Eigen::Matrix4d TEMP = (I - (K * H));
        C_x = TEMP * C_x_predicted * TEMP.transpose() + K * C_nz * K.transpose();
    //}
    std::cout << "////Ping 5d" << std::endl;
}

double RosKalmanFilterNode::distanceMalanovich()
{
    Eigen::Vector2d error_z = z_t - z_predicted;
    Eigen::MatrixXd H_block = H.block<2,2>(0,0);
    Eigen::Matrix2d TEMP = H_block * C_x * H_block.transpose();
    Eigen::Matrix2d inverse = (C_nz + TEMP).inverse();

    distance = (error_z.transpose() * inverse) + error_z.transpose();

    dist = (distance(0) + distance(1));
    std::cout << "//// MALANOVICH dist:" << dist << std::endl;
    return dist;
}

void RosKalmanFilterNode::publish()
{
    kalman_msg_.data.clear();

    std::cout << "////Ping 7a" << std::endl;
    kalman_msg_.data[0] = (float)x_t[0];
    std::cout << "////Ping 7b" << std::endl;
    kalman_msg_.data[1] = (float)x_t[1];
    kalman_msg_.data[2] = (float)x_t[2];
    kalman_msg_.data[3] = (float)x_t[3];

    kalman_publi.publish(kalman_msg_);
}

double RosKalmanFilterNode::getRate()
{
    return rate_;
}

void RosKalmanFilterNode::centerFacePixelsCallbacks(const std_msgs::Float32MultiArrayConstPtr& _msg)
{
    try
    {
        z_t[0] = _msg -> data[0];
        z_t[1] = _msg -> data[1];
        z_t[2] = _msg -> data[2];
        z_t[3] = _msg -> data[3];
    }
    catch (ros::Exception& e)
    {
        ROS_ERROR("RosKalmanFilterNode::centerFacePixelsCallbacks(): exception: %s", e.what());
        return;
    }

}
