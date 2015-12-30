#include <iostream>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <algorithm> // std::remove_if
#include <stdexcept>
#include <cmath>

#include "../include/ptz.hpp"

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

void clear() {
  // CSI[2J clears screen, CSI[H moves the cursor to top-left corner
  std::cout << "\x1B[2J\x1B[H";
}

bool invalidChar(char c) { return !(c >= 0 && c < 128); }
void stripUnicode(std::string &str) {
  str.erase(std::remove_if(str.begin(), str.end(), invalidChar), str.end());
}

bool String2Int(const std::string &str, int &result) {
  try {
    std::size_t lastChar;
    result = std::stoi(str, &lastChar, 10);
    return lastChar == str.size();
  } catch (std::invalid_argument &) {
    std::cout << "String2Int-->invalid arg" << std::endl;
    return false;
  } catch (std::out_of_range &) {
    std::cout << "String2Int-->out of range" << std::endl;
    return false;
  }
}

bool String2Float(const std::string &str, float &result) {
  try {
    std::size_t lastChar;
    result = std::stof(str, &lastChar);
    return lastChar == str.size();
  } catch (std::invalid_argument &) {
    std::cout << "String2Float-->invalid arg" << std::endl;
    return false;
  } catch (std::out_of_range &) {
    std::cout << "String2Float-->out of range" << std::endl;
    return false;
  }
}

Axis6045::Axis6045(std::string ip, std::string usr, std::string pw) {
  std::cout << "Axis6045 IP overloaded constructor called." << std::endl;
  ip_ = ip;
  pw_ = pw;
  usr_ = usr;

  curl_global_init(CURL_GLOBAL_ALL);
  curl_ = curl_easy_init();
}

Axis6045::~Axis6045() {
  std::cout << "Axis6045 default destructor called." << std::endl;

  curl_easy_cleanup(curl_);
  curl_global_cleanup();
}

void Axis6045::ShowInfo() {
  std::cout << "### Camera-> AXIS Information ###" << std::endl;
  std::cout << "Target CCTV-IP: " << ip_ << std::endl;
  std::cout << "position->pan_=" << pan_ << std::endl;
  std::cout << "position->setpoint_pan_=" << setpoint_pan_ << std::endl;
  std::cout << "position->setpoint_tilt_=" << setpoint_tilt_ << std::endl;
  std::cout << "position->setpoint_zoom_=" << setpoint_zoom_ << std::endl;

  std::cout << "position->delta_pan_=" << delta_pan_ << std::endl;
  std::cout << "position->delta_tilt_=" << delta_tilt_ << std::endl;
  std::cout << "position->delta_zoom_=" << delta_zoom_ << std::endl;


  std::cout << "position->tilt_=" << tilt_ << std::endl;
  std::cout << "position->zoom_=" << zoom_ << std::endl;
  std::cout << "position->iris_=" << iris_ << std::endl;
  std::cout << "position->focus_=" << focus_ << std::endl;
  std::cout << "position->autofocus_=" << autofocus_ << std::endl;
  std::cout << "position->autoiris_=" << autoiris_ << std::endl;
  std::cout << "camera_=" << camera_ << std::endl;
}

void Axis6045::SetPassword() {
  // Get password
  std::cout << "Enter ptz password for AXIS camera:" << std::endl;
  termios oldt;
  tcgetattr(STDIN_FILENO, &oldt);
  termios newt = oldt;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  std::string s_password;
  getline(std::cin, s_password);
  // std::cout << s << endl;
  pw_ = s_password;
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  std::cout << "Password stored in instance." << std::endl;
}

void Axis6045::SetPassword(std::string s_password) {
  pw_ = s_password;
  std::cout << "Password stored in instance." << std::endl;
}

void Axis6045::RefreshPosition() {
  std::string query_string = "?query=position";
  std::string response_string = "";
  Axis6045::QueryCamera_(query_string, response_string);
  Axis6045::UpdatePosition_(response_string);
  std::cout << "RefreshPosition() finished." << std::endl;
}

void Axis6045::UpdatePosition_(std::string &html_response) {
  std::istringstream ss(html_response);
  std::string token;
  std::string SplitVec; // #2: Search for tokens

  uint count = 0;

  while (std::getline(ss, token, '\n')) {
    std::cout << "<-" << token << std::endl;
    boost::algorithm::trim_right(token); // Remove newline and spaces at the end
    if (token.find('=') != std::string::npos) {
      std::string first_element = token.substr(0, token.find('='));
      std::string second_element = token.substr(token.find('=') + 1);

      if (first_element == "pan") {
        String2Float(second_element, (float &)pan_);
        delta_pan_ = setpoint_pan_ - pan_;
      } else if (first_element == "tilt") {
        String2Float(second_element, (float &)tilt_);
        delta_tilt_ = setpoint_tilt_ - tilt_;
      } else if (first_element == "zoom") {
        String2Int(second_element, (int &)zoom_);
        delta_zoom_ = setpoint_zoom_ - zoom_;
      } else if (first_element == "iris") {
        String2Int(second_element, (int &)iris_);
      } else if (first_element == "focus") {
        String2Int(second_element, (int &)focus_);
      } else if (first_element == "autofocus") {
        if (second_element == "on") {
          autofocus_ = true;
        } else {
          autofocus_ = false;
        }
      } else if (first_element == "autoiris") {
        if (second_element == "on") {
          autoiris_ = true;
        } else {
          autoiris_ = false;
        }
      } else {
        std::cout << "Unknown position element!" << std::endl;
      }
    } else {
      std::cout << "line is not valid key=value pair:" << token << std::endl;
    }
    count++;
  }
  std::cout << "<- Count of position elements: " << count << std::endl;
}

bool Axis6045::QueryCamera_(const std::string query_string,
                            std::string &response_string) {

  std::string full_query_string = "http://192.168.0.120/axis-cgi/com/ptz.cgi?" +
                                  std::to_string(camera_) + "&query=position";
  std::cout << "Command camera called" << std::endl;
  if (curl_) {
    curl_easy_setopt(curl_, CURLOPT_URL, full_query_string.c_str());
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, "Mozilla/4.0");
    curl_easy_setopt(curl_, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    curl_easy_setopt(curl_, CURLOPT_USERNAME, usr_.c_str());
    curl_easy_setopt(curl_, CURLOPT_PASSWORD, pw_.c_str());
    curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 0L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 20L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 10L);

    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);

    res_ = curl_easy_perform(curl_);
    if (res_ != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res_));
    }
  }
  return false;
}

void Axis6045::StepTowardsSetpoint(){
  std::string ptz_string;
  if (fabs(setpoint_pan_ - pan_)>0.1) {
    ptz_string.append("&pan=" + std::to_string(setpoint_pan_));
  }
   if (fabs(setpoint_tilt_ - tilt_)>0.1) {
    ptz_string.append("&tilt=" + std::to_string(setpoint_tilt_));
  }
  
   if (abs(setpoint_zoom_ - zoom_)>1) {
    ptz_string.append("&zoom=" + std::to_string(setpoint_tilt_));
  }
  std::cout << ptz_string << std::endl;
  CommandCamera_(ptz_string);
}

void Axis6045::SetpointPanTiltZoom(float pan, float tilt, uint zoom) {
  setpoint_pan_ = pan;
  setpoint_tilt_ = tilt;
  setpoint_zoom_ = zoom;
}

bool Axis6045::CommandCamera_(const std::string query_string) {
  std::string full_query_string = "http://192.168.0.120/axis-cgi/com/ptz.cgi?" +
                                  std::to_string(camera_) + query_string;
  std::cout << "Command camera called" << std::endl;
  if (curl_) {
    curl_easy_setopt(curl_, CURLOPT_URL, full_query_string.c_str());
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, "Mozilla/4.0");
    curl_easy_setopt(curl_, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    curl_easy_setopt(curl_, CURLOPT_USERNAME, usr_.c_str());
    curl_easy_setopt(curl_, CURLOPT_PASSWORD, pw_.c_str());
    curl_easy_setopt(curl_, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 0L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 20L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 10L);

    res_ = curl_easy_perform(curl_);
    if (res_ != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res_));
    }
  }
  return true;
}
