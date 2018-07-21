#include "tag.h"
#include <iostream>
#include <cstdlib>

#include <ape/apetag.h>
#include <mpeg/id3v1/id3v1tag.h>
#include <mpeg/id3v2/frames/popularimeterframe.h>
#include <mpeg/id3v2/frames/textidentificationframe.h>
#include <mpeg/id3v2/id3v2frame.h>
#include <mpeg/id3v2/id3v2header.h>
#include <mpeg/id3v2/id3v2tag.h>
#include <mpeg/mpegfile.h>

#include <fmt/format.h>

using namespace std;
using namespace TagLib;

void createOrUpdateUserTextField(
  MPEG::File& f, ID3v2::Tag* id3v2tag, const string& frameIdStr,
  const string& descriptionStr, const string& ratingStr);

void createOrUpdatePopularityMeter(
  MPEG::File& f, ID3v2::Tag* id3v2tag, int ratingInt);

void dump(std::string& path)
{

  MPEG::File f(path.c_str());

  ID3v2::Tag* id3v2tag = f.ID3v2Tag();

  if (id3v2tag) {

    cout << "ID3v2." << id3v2tag->header()->majorVersion() << "."
         << id3v2tag->header()->revisionNumber() << ", "
         << id3v2tag->header()->tagSize() << " bytes in tag" << endl;

    auto it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
      cout << (*it)->frameID() << " - \"" << (*it)->toString() << "\"" << endl;
  }
  else
    cout << "file does not have a valid id3v2 tag" << endl;

  cout << endl << "ID3v1" << endl;

  ID3v1::Tag* id3v1tag = f.ID3v1Tag();

  if (id3v1tag) {
    cout << "title   - \"" << id3v1tag->title() << "\"" << endl;
    cout << "artist  - \"" << id3v1tag->artist() << "\"" << endl;
    cout << "album   - \"" << id3v1tag->album() << "\"" << endl;
    cout << "year    - \"" << id3v1tag->year() << "\"" << endl;
    cout << "comment - \"" << id3v1tag->comment() << "\"" << endl;
    cout << "track   - \"" << id3v1tag->track() << "\"" << endl;
    cout << "genre   - \"" << id3v1tag->genre() << "\"" << endl;
  }
  else
    cout << "file does not have a valid id3v1 tag" << endl;

  APE::Tag* ape = f.APETag();

  cout << endl << "APE" << endl;

  if (ape) {
    for (auto it = ape->itemListMap().begin(); it != ape->itemListMap().end();
         ++it) {
      if ((*it).second.type() != APE::Item::Binary)
        cout << (*it).first << " - \"" << (*it).second.toString() << "\""
             << endl;
      else
        cout << (*it).first << " - Binary data ("
             << (*it).second.binaryData().size() << " bytes)" << endl;
    }
  }
  else
    cout << "file does not have a valid APE tag" << endl;

  cout << endl;
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
      if (
        ((ID3v2::UserTextIdentificationFrame*)(*it))->description()
        == "RD-RATING") {
        const auto& fl =
          ((ID3v2::UserTextIdentificationFrame*)(*it))->fieldList();
        return atoi(fl[1].toCString());
      }
    }
  }

  return -1;
}

// TXXX - "[RD-RATING] RD-RATING 3"
void setMyRating(const string& path, int rating)
{
  MPEG::File f(path.c_str());
  ID3v2::Tag* id3v2tag = f.ID3v2Tag();

  if (!id3v2tag) {
    fmt::print("NO ID3V2TAG\n");
    return;
  }

  createOrUpdateUserTextField(
    f, id3v2tag, "TXXX", "RD-RATING", fmt::format("{}", rating));
  createOrUpdateUserTextField(
    f, id3v2tag, "TXXX", "FMPS_Rating", fmt::format("{:.1f}", rating / 5.0));
  createOrUpdatePopularityMeter(f, id3v2tag, rating);
}

void createOrUpdateUserTextField(
  MPEG::File& f, ID3v2::Tag* id3v2tag, const string& frameIdStr,
  const string& descriptionStr, const string& ratingStr)
{
  bool foundFrame = false;
  auto it = id3v2tag->frameList().begin();
  for (; it != id3v2tag->frameList().end(); it++) {
    if ((*it)->frameID() == frameIdStr.c_str()) {
      if (
        ((ID3v2::UserTextIdentificationFrame*)(*it))->description()
        == descriptionStr) {
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

void createOrUpdatePopularityMeter(
  MPEG::File& f, ID3v2::Tag* id3v2tag, int ratingInt)
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

  createOrUpdateUserTextField(
    f, id3v2tag, "TXXX", "FMPS_Rating", fmt::format("{:.1f}", myRating / 5.0));
  createOrUpdatePopularityMeter(f, id3v2tag, myRating);
}

void StripID3v1(const string& path)
{
  MPEG::File f(path.c_str());
  f.strip(MPEG::File::ID3v1);
}
