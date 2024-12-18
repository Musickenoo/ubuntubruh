#include <ESP32Servo.h>
#include <micro_ros_arduino.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/bool.h>
#include <example_interfaces/srv/set_bool.h>

#define MICRO_ROS_DOMAIN_ID 30

rcl_allocator_t allocator;
rclc_executor_t executor;
rclc_support_t support;
rcl_node_t node;
rcl_subscription_t power_sub;
rcl_service_t toggle_servo_service;
std_msgs__msg__Bool power_msg;
example_interfaces__srv__SetBool_Request req;
example_interfaces__srv__SetBool_Response res;

bool sta = false;
Servo myservo; // Declare a variable to control the servo
Servo myservo1; // Declare a variable to control the servo

enum states {
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED
} state = WAITING_AGENT;

enum servo_states {
  STOPPED,
  ROTATING_LEFT,
  WAITING_LEFT,
  ROTATING_RIGHT,
  WAITING_RIGHT,
  MOVING_TO_0,
  MOVING_TO_01,
  MOVING_TO_180
} servo_state = STOPPED;

unsigned long servo_timer = 0;
const unsigned long right_duration = 500;
const unsigned long stop_duration = 700;
const unsigned long left_duration = 500;
const unsigned long maloat_duration = 1500;

void power_callback(const void * msgin) {
  const std_msgs__msg__Bool * msg = (const std_msgs__msg__Bool *)msgin;
  sta = msg->data;
}

void toggle_servo_callback(const void * req, void * res) {
  const example_interfaces__srv__SetBool_Request * request = (const example_interfaces__srv__SetBool_Request *)req;
  example_interfaces__srv__SetBool_Response * response = (example_interfaces__srv__SetBool_Response *)res;
  
  if (request->data) {
    sta = true;
    response->success = true;
  } else {
    sta = false;
    response->success = true;
  }
}

bool create_entities() {
  allocator = rcl_get_default_allocator();

  // Initialize support with domain ID
  rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
  rcl_init_options_init(&init_options, allocator);
  rcl_init_options_set_domain_id(&init_options, MICRO_ROS_DOMAIN_ID);

  if (rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator) != RCL_RET_OK) {
    Serial.println("Failed to initialize support.");
    return false;
  }

  // Create node
  if (rclc_node_init_default(&node, "servo_node", "", &support) != RCL_RET_OK) {
    Serial.println("Failed to initialize node.");
    return false;
  }

  // Create subscription
  if (rclc_subscription_init_default(
      &power_sub,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
      "power_sub") != RCL_RET_OK) {
    Serial.println("Failed to initialize subscription.");
    return false;
  }

  // Create service
  if (rclc_service_init_default(
      &toggle_servo_service,
      &node,
      ROSIDL_GET_SRV_TYPE_SUPPORT(example_interfaces, srv, SetBool),
      "toggle_servo") != RCL_RET_OK) {
    Serial.println("Failed to initialize service.");
    return false;
  }

  // Initialize executor
  if (rclc_executor_init(&executor, &support.context, 2, &allocator) != RCL_RET_OK) {
    Serial.println("Failed to initialize executor.");
    return false;
  }

  // Add subscription to executor
  if (rclc_executor_add_subscription(&executor, &power_sub, &power_msg, &power_callback, ON_NEW_DATA) != RCL_RET_OK) {
    Serial.println("Failed to add subscription to executor.");
    return false;
  }

  // Add service to executor
  if (rclc_executor_add_service(&executor, &toggle_servo_service, &req, &res, &toggle_servo_callback) != RCL_RET_OK) {
    Serial.println("Failed to add service to executor.");
    return false;
  }

  Serial.println("micro-ROS entities created successfully.");
  state = AGENT_CONNECTED;
  return true;
}

void destroy_entities() {
  rcl_subscription_fini(&power_sub, &node);
  rcl_service_fini(&toggle_servo_service, &node);
  rcl_node_fini(&node);
  rclc_executor_fini(&executor);
  rclc_support_fini(&support);

  Serial.println("micro-ROS entities destroyed.");
  state = AGENT_DISCONNECTED;
}

void handle_reconnection() {
  if (state == AGENT_DISCONNECTED) {
    Serial.println("Reconnecting to micro-ROS agent...");
    destroy_entities();
    delay(1000); // Wait for 1 second before trying to reconnect
    if (create_entities()) {
      Serial.println("Reconnection attempt successful.");
    } else {
      Serial.println("Reconnection attempt failed.");
      delay(5000); // Wait longer before the next reconnection attempt to prevent flooding
    }
  }
}

void setup() {
  Serial.begin(115200); // Initialize serial communication for debugging

  // Initialize micro-ROS transports
  set_microros_transports();

  myservo.attach(17); // Attach the servo to pin 17
  myservo1.attach(19);
  myservo1.write(0);

  if (!create_entities()) {
    Serial.println("Failed to initialize micro-ROS entities.");
  }
}

void loop() {
  if (state == AGENT_CONNECTED) {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  
    if (sta) {
      unsigned long current_time = millis();

      switch (servo_state) {
        case STOPPED:
          myservo.writeMicroseconds(1500); // Command servo to rotate right
          servo_timer = current_time;
          servo_state = ROTATING_LEFT;
          break;
        
        case ROTATING_LEFT:
          if (current_time - servo_timer >= left_duration) {
            myservo.writeMicroseconds(3000); // Command servo to stop
            servo_timer = current_time;
            servo_state = WAITING_LEFT;
          }
          break;
        
        case WAITING_LEFT:
          if (current_time - servo_timer >= stop_duration) {
            myservo.writeMicroseconds(1450); // Command servo to rotate left
            servo_timer = current_time;
            servo_state = MOVING_TO_180;
          }
          break;

        case MOVING_TO_180:
          if (current_time - servo_timer >= maloat_duration) {
            myservo1.write(180); // Command the second servo to position 180
            servo_timer = current_time;
            servo_state = MOVING_TO_01;
          }
          break;
        
        case MOVING_TO_01:
          if (current_time - servo_timer >= maloat_duration) {
            myservo1.write(0); // Command the second servo to position 0
            servo_timer = current_time;
            servo_state = ROTATING_RIGHT;
          }
          break;

        case ROTATING_RIGHT:
          if (current_time - servo_timer >= right_duration) {
            myservo.writeMicroseconds(0); // Command servo to stop
            servo_timer = current_time;
            servo_state = WAITING_RIGHT;
          }
          break;

        case WAITING_RIGHT:
          if (current_time - servo_timer >= stop_duration) {
            servo_state = STOPPED;
            sta = false; // Reset state to stop servo actions
          }
          break;
      }
    } else {
      myservo.writeMicroseconds(1500); // Keep servo stopped if not active
    }
  } else {
    handle_reconnection();
  }
}
