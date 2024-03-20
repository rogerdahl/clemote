#include <iostream>

#include <fmt/format.h>

#include <taglib/apetag.h>
#include <taglib/id3v1tag.h>
#include <taglib/popularimeterframe.h>
#include <taglib/textidentificationframe.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2header.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>

#include "tag.h"

using namespace std;
using namespace TagLib;

void createOrUpdateUserTextField(
  MPEG::File& f, ID3v2::Tag* id3v2tag, const string& frameIdStr, const string& descriptionStr, const string& ratingStr);

void createOrUpdatePopularityMeter(MPEG::File& f, ID3v2::Tag* id3v2tag, int ratingInt);

void dumpId3V1(MPEG::File& f);
void dumpId3V2(MPEG::File& f);
void dumpApe(MPEG::File& f);

void dump(std::string& path)
{
  MPEG::File f(path.c_str());
  dumpId3V1(f);
  dumpId3V2(f);
  dumpApe(f);
  fmt::print("\n");
}

void dumpId3V1(MPEG::File& f)
{
  ID3v1::Tag* id3v1tag = f.ID3v1Tag();
  if (!id3v1tag) {
    fmt::print("No ID3v1 tag...\n");
    return;
  }
  fmt::print("ID3v1:\n");
  fmt::print("title: {}\n", id3v1tag->title().toCString());
  fmt::print("artist: {}\n", id3v1tag->artist().toCString());
  fmt::print("album: {}\n", id3v1tag->album().toCString());
  fmt::print("year: {}\n", id3v1tag->year());
  fmt::print("comment: {}\n", id3v1tag->comment().toCString());
  fmt::print("track: {}\n", id3v1tag->track());
  fmt::print("genre: {}\n", id3v1tag->genre().toCString());
}

void dumpId3V2(MPEG::File& f)
{
  ID3v2::Tag* id3v2tag = f.ID3v2Tag();
  if (!id3v2tag) {
    fmt::print("No ID3v2 tag...\n");
    return;
  }
  ID3v2::Header* h = id3v2tag->header();
  fmt::print("ID3v2.{}.{}, {} bytes in tag:\n", h->majorVersion(), h->revisionNumber(), h->tagSize());
  for (const auto& it : id3v2tag->frameList()) {
    fmt::print("{} - \"{}\"\n", (it->frameID(), it->toString().toCString()));
  }
}

void dumpApe(MPEG::File& f)
{
  APE::Tag* ape = f.APETag();
  if (!ape) {
    fmt::print("No APE tag...\n");
    return;
  }
  fmt::print("APE:\n");
  for (const auto& it : ape->itemListMap()) {
    if (it.second.type() != APE::Item::Binary)
      cout << it.first << " - \"" << it.second.toString() << "\"" << endl;
    else
      cout << it.first << " - Binary data (" << it.second.binaryData().size() << " bytes)" << endl;
  }
}

// TXXX - "[RD-RATING] RD-RATING 3"
int getMyRating(const string& path)
{
  MPEG::File f(path.c_str());
  ID3v2::Tag* id3v2tag = f.ID3v2Tag();

  if (!id3v2tag) {
    fmt::print("NO ID3V2TAG\n");
    return -1;
  }

  auto it = id3v2tag->frameList().begin();
  for (; it != id3v2tag->frameList().end(); it++) {
    if ((*it)->frameID() == "TXXX") {
      if (((ID3v2::UserTextIdentificationFrame*)(*it))->description() == "RD-RATING") {
        const auto& fl = ((ID3v2::UserTextIdentificationFrame*)(*it))->fieldList();
        return fl[1].toInt();
      }
    }
  }

  return -1;
}

// TXXX - "[RD-RATING] RD-RATING 3"
void setMyRating(const string& path, int rating)
{
  MPEG::File f(path.c_str());
  ID3v2::Tag* id3v2tag = f.ID3v2Tag(true);

  // if (!id3v2tag) {
  //   fmt::print("NO ID3V2TAG\n");
  //   return;
  // }

  createOrUpdateUserTextField(f, id3v2tag, "TXXX", "RD-RATING", fmt::format("{}", rating));
  createOrUpdateUserTextField(f, id3v2tag, "TXXX", "FMPS_Rating", fmt::format("{:.1f}", rating / 5.0));
  createOrUpdatePopularityMeter(f, id3v2tag, rating);
}

void createOrUpdateUserTextField(
  MPEG::File& f, ID3v2::Tag* id3v2tag, const string& frameIdStr, const string& descriptionStr, const string& ratingStr)
{
  bool foundFrame = false;
  auto it = id3v2tag->frameList().begin();
  for (; it != id3v2tag->frameList().end(); it++) {
    if ((*it)->frameID() == frameIdStr.c_str()) {
      if (((ID3v2::UserTextIdentificationFrame*)(*it))->description() == descriptionStr) {
        foundFrame = true;
        (*it)->setText(ratingStr);
        break;
      }
    }
  }
  if (!foundFrame) {
    auto frame = new ID3v2::UserTextIdentificationFrame(frameIdStr.c_str());
    frame->setDescription(descriptionStr);
    frame->setText(ratingStr);
    id3v2tag->addFrame(frame);
  }
  f.save();
}

void createOrUpdatePopularityMeter(MPEG::File& f, ID3v2::Tag* id3v2tag, int ratingInt)
{
  ratingInt *= 51;
  bool foundFrame = false;
  auto it = id3v2tag->frameList().begin();
  for (; it != id3v2tag->frameList().end(); it++) {
    if ((*it)->frameID() == "POPM") {
      foundFrame = true;
      ((ID3v2::PopularimeterFrame*)(*it))->setRating(ratingInt);
      break;
    }
  }
  if (!foundFrame) {
    auto frame = new ID3v2::PopularimeterFrame("POPM");
    frame->setRating(ratingInt);
    id3v2tag->addFrame(frame);
  }

  f.save();
}

void syncMyRatingToPopularityMeter(const std::string& path)
{
  int myRating = getMyRating(path);
  if (myRating < 0) {
    return;
  }

  fmt::print("Sync: rating={} path=\"{}\"\n", myRating, path);

  MPEG::File f(path.c_str());
  ID3v2::Tag* id3v2tag = f.ID3v2Tag();

  createOrUpdateUserTextField(f, id3v2tag, "TXXX", "FMPS_Rating", fmt::format("{:.1f}", myRating / 5.0));
  createOrUpdatePopularityMeter(f, id3v2tag, myRating);
}

void StripID3v1(const string& path)
{
  MPEG::File f(path.c_str());
  f.strip(MPEG::File::ID3v1);
}
