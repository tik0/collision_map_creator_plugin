#include <iostream>
#include <math.h>
#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/png_dynamic_io.hpp>
#include <boost/shared_ptr.hpp>
#include <sdf/sdf.hh>

#include "gazebo/gazebo.hh"
#include "gazebo/common/common.hh"
#include "gazebo/math/Vector3.hh"
#include "gazebo/msgs/msgs.hh"
#include "gazebo/physics/physics.hh"
#include "gazebo/transport/transport.hh"
#include "collision_map_request.pb.h"

namespace gazebo
{
typedef const boost::shared_ptr<
  const collision_map_creator_msgs::msgs::CollisionMapRequest>
    CollisionMapRequestPtr;

class CollisionMapCreator : public WorldPlugin
{
  transport::NodePtr node;
  transport::PublisherPtr imagePub;
  transport::SubscriberPtr commandSubscriber;
  physics::WorldPtr world;

  public: void Load(physics::WorldPtr _parent, sdf::ElementPtr _sdf)
  {
    node = transport::NodePtr(new transport::Node());
    world = _parent;
    // Initialize the node with the world name
    node->Init(world->GetName());
    std::cout << "Subscribing to: " << "~/collision_map/command" << std::endl;
    commandSubscriber = node->Subscribe("~/collision_map/command",
      &CollisionMapCreator::create, this);
    imagePub = node->Advertise<msgs::Image>("~/collision_map/image");
  }

  public: void create(CollisionMapRequestPtr &msg)
  {
    std::cout << "Received message" << std::endl;

    std::cout << "Creating collision map with corners at (" <<
      msg->lowerleft().x() << ", " << msg->lowerleft().y() << "), (" <<
      msg->upperright().x() << ", " << msg->upperright().y() <<
        ") with collision projected from z = " <<
      msg->height() << "\nResolution = " << msg->resolution() << " m\n" <<
        "Occupied spaces will be filled with: " << msg->threshold() <<
        std::endl;

    std::string groundEntityName;
    if (!msg->groundentityname().empty())
    {
      groundEntityName = msg->groundentityname();
      std::cout << "Entity \"" << groundEntityName << "\" is treated as ground." << std::endl;
    }

    double dX = msg->upperright().x() - msg->lowerleft().x();
    double dY = msg->upperright().y() - msg->lowerleft().y();

    std::cout << "(dX, dY): (" << dX << ", " << dY << ")" << std::endl;

    int dX_count = dX / msg->resolution();
    int dY_count = dY / msg->resolution();

    if (dX_count <= 0 || dY_count <= 0)
    {
      std::cerr << "Image has zero or negative dimension, check coordinates"
                << std::endl;
      return;
    }
    double x,y;

    boost::gil::gray8_pixel_t fill(255-msg->threshold());
    boost::gil::gray8_pixel_t blank(255);
    boost::gil::gray8_image_t image(dX_count, dY_count);

    double dist;
    std::string entityName;
    math::Vector3 start, end;
    start.z = msg->height();
    end.z = msg->minheight();

    gazebo::physics::PhysicsEnginePtr engine = world->GetPhysicsEngine();
    engine->InitForThread();
    gazebo::physics::RayShapePtr ray =
      boost::dynamic_pointer_cast<gazebo::physics::RayShape>(
        engine->CreateShape("ray", gazebo::physics::CollisionPtr()));

    std::cout << "Rasterizing model and checking collisions" << std::endl;
    boost::gil::fill_pixels(image._view, blank);

    for (int idx = 0; idx < dX_count; ++idx)
    {
      if (dX_count <= 100 || idx % int(dX_count * 0.01f) == 0) // output progess every percent
      {
        std::cout << "Percent complete: " << idx * 100.0 / dX_count << std::endl;
      }

      x = idx * msg->resolution() + msg->lowerleft().x();
      for (int idy = 0; idy < dY_count; ++idy)
      {
        y = idy * msg->resolution() + msg->lowerleft().y();

        start.x = end.x = x;
        start.y = end.y = y;
        ray->SetPoints(start, end);
        ray->GetIntersection(dist, entityName);
        if (entityName != groundEntityName)
        {
          image._view(idx, idy) = fill;
        }
      }
    }

    std::cout << "Completed calculations, writing to image ...";
    if (!msg->filename().empty())
    {
      boost::gil::gray8_view_t view = image._view;
      boost::gil::png_write_view(msg->filename(), view);
    }
    std::cout << " done" << std::endl;
  }
};

// Register this plugin with the simulator
GZ_REGISTER_WORLD_PLUGIN(CollisionMapCreator)
}
