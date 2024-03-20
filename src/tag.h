#pragma once

#include <string>

void dump(std::string&);
int getMyRating(const std::string& path);
void setMyRating(const std::string& path, int rating);
void syncMyRatingToPopularityMeter(const std::string& path);
void StripID3v1(const std::string& path);
