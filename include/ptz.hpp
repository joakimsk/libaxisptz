#ifndef __PTZ_HPP_INCLUDED__
#define __PTZ_HPP_INCLUDED__

#include <iostream>
#include <map>
#include <time.h> /* clock_t, clock, CLOCKS_PER_SEC */
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <sstream>

class Axis6045 {
public:
  Axis6045(std::string ip, std::string usr, std::string pw);
  ~Axis6045();
  void ShowInfo();
  void SetPassword();
  void SetPassword(std::string s_password);
  void RefreshPosition();
  void StepTowardsSetpoint();
  void SetpointPanTiltZoom(float pan, float tilt, uint zoom);

protected:
  clock_t cpu_t_of_grab_picture_;

  bool CommandCamera_(const std::string query_string);
  void UpdatePosition_(std::string &html_response);
  bool QueryCamera_(const std::string query_string,
                    std::string &response_string);

private:
  // general camera
  std::string ip_ = "0.0.0.0";
  std::string pw_ = "";
  std::string usr_ = "";
  uint camera_ = 1;

  // position
  float pan_ = 0.0;
  float tilt_ = 0.0;
  uint zoom_ = 0;
  uint iris_ = 0;
  uint focus_ = 0;
  bool autofocus_ = false;
  bool autoiris_ = false;

  float setpoint_pan_ = 0.0;
  float setpoint_tilt_ = 0.0;
  uint setpoint_zoom_ = 1;

  float delta_pan_ = 0.0;
  float delta_tilt_ = 0.0;
  uint delta_zoom_ = 0;

  // limits
  float min_pan_ = 0.0;
  float max_pan_ = 180.0;
  uint min_tilt_ = 0;
  uint max_tilt_ = 0;
  uint min_zoom_ = 0;
  uint max_zoom_ = 0;
  uint min_iris_ = 0;
  uint max_iris_ = 0;
  uint min_focus_ = 0;
  uint max_focus_ = 0;
  uint min_field_angle_ = 0;
  uint max_field_angle_ = 0;
  uint min_brightness_ = 0;
  uint max_brightness_ = 0;

  CURL *curl_;
  CURLcode res_;

  static constexpr const char *QueryPosition = "some useful string constant";
  static constexpr const char *QueryLimits = "some useful string constant";
};
#endif