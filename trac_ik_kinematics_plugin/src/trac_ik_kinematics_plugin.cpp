/********************************************************************************
Copyright (c) 2015, TRACLabs, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, 
       this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software 
       without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************/


#include <ros/ros.h>
#include <moveit/kinematics_base/kinematics_base.h>
#include <urdf/model.h>
#include <tf_conversions/tf_kdl.h>
#include <algorithm>
#include <kdl/tree.hpp>
#include <kdl/chain.hpp>
#include <kdl_parser/kdl_parser.hpp>
#include <trac_ik/trac_ik.hpp>
#include <limits>


namespace trac_ik_kinematics_plugin
{

  class TRAC_IKKinematicsPlugin : public kinematics::KinematicsBase
  {
    std::vector<std::string> joint_names_;
    std::vector<double> joint_min_vector_;
    std::vector<double> joint_max_vector_;
    std::vector<bool> joint_has_limits_vector_;
    std::vector<std::string> link_names_;

    uint num_joints_;
    bool active_; // Internal variable that indicates whether solvers are configured and ready

    KDL::Chain chain;
    bool position_ik_;
    const std::vector<std::string>& getJointNames() const { return joint_names_; }
    const std::vector<std::string>& getLinkNames() const { return link_names_; }

    KDL::JntArray joint_min, joint_max;

    std::string solve_type;
    std::string free_angle;

  public:

    /** @class
     *  @brief Interface for an TRAC-IK kinematics plugin
     */
    TRAC_IKKinematicsPlugin(): active_(false), position_ik_(false){}

    ~TRAC_IKKinematicsPlugin() {
    }

    /**
     * @brief Given a desired pose of the end-effector, compute the joint angles to reach it
     * @param ik_pose the desired pose of the link
     * @param ik_seed_state an initial guess solution for the inverse kinematics
     * @param solution the solution vector
     * @param error_code an error code that encodes the reason for failure or success
     * @return True if a valid solution was found, false otherwise
     */

    // Returns the first IK solution that is within joint limits, this is called by get_ik() service
    bool getPositionIK(const geometry_msgs::Pose &ik_pose,
                       const std::vector<double> &ik_seed_state,
                       std::vector<double> &solution,
                       moveit_msgs::MoveItErrorCodes &error_code,
                       const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

    /**
     * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
     * This particular method is intended for "searching" for a solutions by stepping through the redundancy
     * (or other numerical routines).
     * @param ik_pose the desired pose of the link
     * @param ik_seed_state an initial guess solution for the inverse kinematics
     * @return True if a valid solution was found, false otherwise
     */
    bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                          const std::vector<double> &ik_seed_state,
                          double timeout,
                          std::vector<double> &solution,
                          moveit_msgs::MoveItErrorCodes &error_code,
                          const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

    /**
     * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
     * This particular method is intended for "searching" for a solutions by stepping through the redundancy
     * (or other numerical routines).
     * @param ik_pose the desired pose of the link
     * @param ik_seed_state an initial guess solution for the inverse kinematics
     * @param the distance that the redundancy can be from the current position
     * @return True if a valid solution was found, false otherwise
     */
    bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                          const std::vector<double> &ik_seed_state,
                          double timeout,
                          const std::vector<double> &consistency_limits,
                          std::vector<double> &solution,
                          moveit_msgs::MoveItErrorCodes &error_code,
                          const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

    /**
     * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
     * This particular method is intended for "searching" for a solutions by stepping through the redundancy
     * (or other numerical routines).
     * @param ik_pose the desired pose of the link
     * @param ik_seed_state an initial guess solution for the inverse kinematics
     * @return True if a valid solution was found, false otherwise
     */
    bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                          const std::vector<double> &ik_seed_state,
                          double timeout,
                          std::vector<double> &solution,
                          const IKCallbackFn &solution_callback,
                          moveit_msgs::MoveItErrorCodes &error_code,
                          const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

    /**
     * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
     * This particular method is intended for "searching" for a solutions by stepping through the redundancy
     * (or other numerical routines).  The consistency_limit specifies that only certain redundancy positions
     * around those specified in the seed state are admissible and need to be searched.
     * @param ik_pose the desired pose of the link
     * @param ik_seed_state an initial guess solution for the inverse kinematics
     * @param consistency_limit the distance that the redundancy can be from the current position
     * @return True if a valid solution was found, false otherwise
     */
    bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                          const std::vector<double> &ik_seed_state,
                          double timeout,
                          const std::vector<double> &consistency_limits,
                          std::vector<double> &solution,
                          const IKCallbackFn &solution_callback,
                          moveit_msgs::MoveItErrorCodes &error_code,
                          const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

    bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                          const std::vector<double> &ik_seed_state,
                          double timeout,
                          std::vector<double> &solution,
                          const IKCallbackFn &solution_callback,
                          moveit_msgs::MoveItErrorCodes &error_code,
                          const std::vector<double> &consistency_limits,
                          const kinematics::KinematicsQueryOptions &options) const;
    


    /**
     * @brief Given a set of joint angles and a set of links, compute their pose
     *
     * This FK routine is only used if 'use_plugin_fk' is set in the 'arm_kinematics_constraint_aware' node,
     * otherwise ROS TF is used to calculate the forward kinematics
     *
     * @param link_names A set of links for which FK needs to be computed
     * @param joint_angles The state for which FK is being computed
     * @param poses The resultant set of poses (in the frame returned by getBaseFrame())
     * @return True if a valid solution was found, false otherwise
     */
    bool getPositionFK(const std::vector<std::string> &link_names,
                       const std::vector<double> &joint_angles,
                       std::vector<geometry_msgs::Pose> &poses) const;

  private:

    bool initialize(const std::string &robot_description,
                    const std::string& group_name,
                    const std::string& base_name,
                    const std::string& tip_name,
                    double search_discretization);


    int getKDLSegmentIndex(const std::string &name) const;

  }; // end class

  bool TRAC_IKKinematicsPlugin::initialize(const std::string &robot_description,
                                           const std::string& group_name,
                                           const std::string& base_name,
                                           const std::string& tip_name,
                                           double search_discretization)
  {
    setValues(robot_description, group_name, base_name, tip_name, search_discretization);

    ros::NodeHandle node_handle("~");
    ros::NodeHandle node_handle_kinematic_descr("/robot_description_kinematics");

    urdf::Model robot_model;
    std::string xml_string;

    std::string urdf_xml,full_urdf_xml;
    node_handle.param("urdf_xml",urdf_xml,robot_description);
    node_handle.searchParam(urdf_xml,full_urdf_xml);

    ROS_DEBUG_NAMED("trac_ik","Reading xml file from parameter server");
    if (!node_handle.getParam(full_urdf_xml, xml_string))
      {
        ROS_FATAL_NAMED("trac_ik","Could not load the xml from parameter server: %s", urdf_xml.c_str());
        return false;
      }

    node_handle.param(full_urdf_xml,xml_string,std::string());
    robot_model.initString(xml_string);

    ROS_DEBUG_STREAM_NAMED("trac_ik","Reading joints and links from URDF");

    boost::shared_ptr<urdf::Link> link = boost::const_pointer_cast<urdf::Link>(robot_model.getLink(getTipFrame()));
    while(link->name != base_frame_)
      {
        ROS_DEBUG_NAMED("trac_ik","Link %s",link->name.c_str());
        link_names_.push_back(link->name);
        boost::shared_ptr<urdf::Joint> joint = link->parent_joint;
        if(joint)
          {
            if (joint->type != urdf::Joint::UNKNOWN && joint->type != urdf::Joint::FIXED)
              {
                ROS_DEBUG_STREAM_NAMED("trac_ik","Adding joint " << joint->name );

                joint_names_.push_back(joint->name);
                float lower, upper;
                int hasLimits;
                if ( joint->type != urdf::Joint::CONTINUOUS )
                  {
                    if(joint->safety)
                      {
                        lower = std::max(joint->limits->lower, joint->safety->soft_lower_limit);
                        upper = std::min(joint->limits->upper, joint->safety->soft_upper_limit);
                      } else {
                      lower = joint->limits->lower;
                      upper = joint->limits->upper;
                    }
                    hasLimits = 1;
                  }
                else
                  {
                    hasLimits = 0;
                  }
                if(hasLimits)
                  {
                    joint_has_limits_vector_.push_back(true);
                    joint_min_vector_.push_back(lower);
                    joint_max_vector_.push_back(upper);
                  }
                else
                  {
                    joint_has_limits_vector_.push_back(false);
                    joint_min_vector_.push_back(-std::numeric_limits<float>::max());
                    joint_max_vector_.push_back(std::numeric_limits<float>::max());
                  }
              }
          } else
          {
            ROS_WARN_NAMED("trac_ik","no joint corresponding to %s",link->name.c_str());
          }
        link = link->getParent();
      }


    num_joints_ = joint_names_.size();

    std::reverse(link_names_.begin(),link_names_.end());
    std::reverse(joint_names_.begin(),joint_names_.end());
    std::reverse(joint_min_vector_.begin(),joint_min_vector_.end());
    std::reverse(joint_max_vector_.begin(),joint_max_vector_.end());
    std::reverse(joint_has_limits_vector_.begin(), joint_has_limits_vector_.end());

    joint_min.resize(num_joints_);
    joint_max.resize(num_joints_);

    for(uint i=0; i <num_joints_; ++i) {
      joint_min(i)=joint_min_vector_[i];
      joint_max(i)=joint_max_vector_[i];
      ROS_INFO_STREAM_NAMED("trac_ik",joint_names_[i] << " " << joint_min_vector_[i] << " " << joint_max_vector_[i] << " " << joint_has_limits_vector_[i]);
    }

    KDL::Tree tree;

    if (!kdl_parser::treeFromUrdfModel(robot_model, tree))
      ROS_FATAL("Failed to extract kdl tree from xml robot description");

    if(!tree.getChain(base_name, tip_name, chain))
      ROS_FATAL("Couldn't find chain %s to %s",base_name.c_str(),tip_name.c_str());

    assert(num_joints_ == chain.getNrOfJoints());

    ROS_INFO_NAMED("trac-ik plugin","Looking in private handle: %s for param name: %s",
                    node_handle_kinematic_descr.getNamespace().c_str(),
                    (group_name+"/position_only_ik").c_str());

    node_handle_kinematic_descr.param(group_name+"/position_only_ik", position_ik_, false);

    ROS_INFO_NAMED("trac-ik plugin","Looking in private handle: %s for param name: %s",
                    node_handle_kinematic_descr.getNamespace().c_str(),
                    (group_name+"/solve_type").c_str());

    node_handle_kinematic_descr.param(group_name+"/solve_type", solve_type, std::string("Speed"));
    ROS_INFO_NAMED("trac_ik plugin","Using solve type %s",solve_type.c_str());

    ROS_INFO_NAMED("trac-ik plugin","Looking in private handle: %s for param name: %s",
                    node_handle_kinematic_descr.getNamespace().c_str(),
                    (group_name+"/free_angle").c_str());

    node_handle_kinematic_descr.param(group_name+"/free_angle", free_angle, std::string(""));
    ROS_INFO_NAMED("trac_ik plugin","Using free angle(s) in IK solution %s",free_angle.c_str());

    active_ = true;
    return true;
  }


  int TRAC_IKKinematicsPlugin::getKDLSegmentIndex(const std::string &name) const
  {
    int i=0;
    while (i < (int)chain.getNrOfSegments()) {
      if (chain.getSegment(i).getName() == name) {
        return i+1;
      }
      i++;
    }
    return -1;
  }


  bool TRAC_IKKinematicsPlugin::getPositionFK(const std::vector<std::string> &link_names,
                                              const std::vector<double> &joint_angles,
                                              std::vector<geometry_msgs::Pose> &poses) const
  {
  if(!active_)
  {
    ROS_ERROR_NAMED("trac_ik","kinematics not active");
    return false;
  }
  poses.resize(link_names.size());
  if(joint_angles.size() != num_joints_)
  {
    ROS_ERROR_NAMED("trac_ik","Joint angles vector must have size: %d",num_joints_);
    return false;
  }

  KDL::Frame p_out;
  geometry_msgs::PoseStamped pose;
  tf::Stamped<tf::Pose> tf_pose;

  KDL::JntArray jnt_pos_in(num_joints_);
  for(unsigned int i=0; i < num_joints_; i++)
  {
    jnt_pos_in(i) = joint_angles[i];
  }

  KDL::ChainFkSolverPos_recursive fk_solver(chain);

  bool valid = true;
  for(unsigned int i=0; i < poses.size(); i++)
  {
    ROS_DEBUG_NAMED("trac_ik","End effector index: %d",getKDLSegmentIndex(link_names[i]));
    if(fk_solver.JntToCart(jnt_pos_in,p_out,getKDLSegmentIndex(link_names[i])) >=0)
    {
      tf::poseKDLToMsg(p_out,poses[i]);
    }
    else
    {
      ROS_ERROR_NAMED("trac_ik","Could not compute FK for %s",link_names[i].c_str());
      valid = false;
    }
  }

  return valid;
}


  bool TRAC_IKKinematicsPlugin::getPositionIK(const geometry_msgs::Pose &ik_pose,
                                          const std::vector<double> &ik_seed_state,
                                          std::vector<double> &solution,
                                          moveit_msgs::MoveItErrorCodes &error_code,
                                          const kinematics::KinematicsQueryOptions &options) const
  {
    const IKCallbackFn solution_callback = 0;
    std::vector<double> consistency_limits;

    return searchPositionIK(ik_pose,
                            ik_seed_state,
                            default_timeout_,
                            solution,
                            solution_callback,
                            error_code,
                            consistency_limits,
                            options);
  }

  bool TRAC_IKKinematicsPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                             const std::vector<double> &ik_seed_state,
                                             double timeout,
                                             std::vector<double> &solution,
                                             moveit_msgs::MoveItErrorCodes &error_code,
                                             const kinematics::KinematicsQueryOptions &options) const
  {
    const IKCallbackFn solution_callback = 0;
    std::vector<double> consistency_limits;

    return searchPositionIK(ik_pose,
                            ik_seed_state,
                            timeout,
                            solution,
                            solution_callback,
                            error_code,
                            consistency_limits,
                            options);
  }

  bool TRAC_IKKinematicsPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                             const std::vector<double> &ik_seed_state,
                                             double timeout,
                                             const std::vector<double> &consistency_limits,
                                             std::vector<double> &solution,
                                             moveit_msgs::MoveItErrorCodes &error_code,
                                             const kinematics::KinematicsQueryOptions &options) const
  {
    const IKCallbackFn solution_callback = 0;
    return searchPositionIK(ik_pose,
                            ik_seed_state,
                            timeout,
                            solution,
                            solution_callback,
                            error_code,
                            consistency_limits,
                            options);
  }

  bool TRAC_IKKinematicsPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                             const std::vector<double> &ik_seed_state,
                                             double timeout,
                                             std::vector<double> &solution,
                                             const IKCallbackFn &solution_callback,
                                             moveit_msgs::MoveItErrorCodes &error_code,
                                             const kinematics::KinematicsQueryOptions &options) const
  {
    std::vector<double> consistency_limits;
    return searchPositionIK(ik_pose,
                            ik_seed_state,
                            timeout,
                            solution,
                            solution_callback,
                            error_code,
                            consistency_limits,
                            options);
  }

  bool TRAC_IKKinematicsPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                             const std::vector<double> &ik_seed_state,
                                             double timeout,
                                             const std::vector<double> &consistency_limits,
                                             std::vector<double> &solution,
                                             const IKCallbackFn &solution_callback,
                                             moveit_msgs::MoveItErrorCodes &error_code,
                                             const kinematics::KinematicsQueryOptions &options) const
  {
    return searchPositionIK(ik_pose,
                            ik_seed_state,
                            timeout,
                            solution,
                            solution_callback,
                            error_code,
                            consistency_limits,
                            options);
  }

  bool TRAC_IKKinematicsPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                             const std::vector<double> &ik_seed_state,
                                             double timeout,
                                             std::vector<double> &solution,
                                             const IKCallbackFn &solution_callback,
                                             moveit_msgs::MoveItErrorCodes &error_code,
                                             const std::vector<double> &consistency_limits,
                                             const kinematics::KinematicsQueryOptions &options) const
  {
    ROS_DEBUG_STREAM_NAMED("trac_ik","getPositionIK");

    if(!active_) {
      ROS_ERROR("kinematics not active");
      error_code.val = error_code.NO_IK_SOLUTION;
      return false;
    }

    if (ik_seed_state.size() != num_joints_) {
      ROS_ERROR_STREAM_NAMED("trac_ik","Seed state must have size " << num_joints_ << " instead of size " << ik_seed_state.size());
      error_code.val = error_code.NO_IK_SOLUTION;
      return false;
    }

    KDL::Frame frame;
    tf::poseMsgToKDL(ik_pose,frame);

    KDL::JntArray in(num_joints_), out(num_joints_);

    for (uint z=0; z< num_joints_; z++)
        in(z)=ik_seed_state[z];

    KDL::Twist bounds=KDL::Twist::Zero();

    if (options.return_approximate_solution)
    {
        // 5mm for translation
        bounds.vel.x(5e-3);
        bounds.vel.y(5e-3);
        bounds.vel.z(5e-3);

        // ~0.5 Degree for rotation
        bounds.rot.x(1e-2);
        bounds.rot.y(1e-2);
        bounds.rot.z(1e-2);
    }
    
    if (position_ik_)
    {
      bounds.rot.x(std::numeric_limits<float>::max());
      bounds.rot.y(std::numeric_limits<float>::max());
      bounds.rot.z(std::numeric_limits<float>::max());
    }

    if (!free_angle.empty())
    {
        if (free_angle.find("X") != std::string::npos)
            bounds.rot.x(FLT_MAX);
        if (free_angle.find("Y") != std::string::npos)
            bounds.rot.y(FLT_MAX);
        if (free_angle.find("Z") != std::string::npos)
            bounds.rot.z(FLT_MAX);
    }
    
    double epsilon = 1e-5;  //Same as MoveIt's KDL plugin

    TRAC_IK::SolveType solvetype;

    if (solve_type == "Manipulation1")
      solvetype = TRAC_IK::Manip1;
    else if (solve_type == "Manipulation2")
      solvetype = TRAC_IK::Manip2;
    else if (solve_type == "Distance")
      solvetype = TRAC_IK::Distance;
    else solvetype = TRAC_IK::Speed;
    
    TRAC_IK::TRAC_IK ik_solver(chain, joint_min, joint_max, timeout, epsilon, solvetype);

    int rc = ik_solver.CartToJnt(in, frame, out, bounds);


    solution.resize(num_joints_);

    if (rc >=0) {
      for (uint z=0; z< num_joints_; z++)
        solution[z]=out(z);

      // check for collisions if a callback is provided 
      if( !solution_callback.empty() )
        {
          solution_callback(ik_pose, solution, error_code);
          if(error_code.val == moveit_msgs::MoveItErrorCodes::SUCCESS)
            {
              ROS_DEBUG_STREAM_NAMED("trac_ik","Solution passes callback");
              return true;
            }
          else
            {
              ROS_DEBUG_STREAM_NAMED("trac_ik","Solution has error code " << error_code);
              return false;
            }
        }
      else
          return true; // no collision check callback provided
    }

    error_code.val = moveit_msgs::MoveItErrorCodes::NO_IK_SOLUTION;
    return false;
  }



} // end namespace

//register TRAC_IKKinematicsPlugin as a KinematicsBase implementation
#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(trac_ik_kinematics_plugin::TRAC_IKKinematicsPlugin, kinematics::KinematicsBase);
