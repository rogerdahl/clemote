#!/usr/bin/env python3

# import pillow
from PIL import Image, ImageDraw, ImageFont
import PIL.Image
import pprint

LABEL_LIST = [
  ('Led', None, (1512, 238)),
  ('Power', None, (1757, 277)),
  ('Pause', 'Pause', (1508, 370)),
  ('Rec', 'DELETE FILE', (1326, 469)),
  ('Stop', 'Stop', (1692, 464)),
  ('Rwd', '30 sec back', (1152, 604)),
  ('Play', None, (1508, 603)),
  ('Fwd', '30 sec fwd', (1867, 624)),
  ('Prev', 'Prev', (1316, 737)),
  ('Next', 'Next', (1694, 745)),
  ('Exit', 'Prev', (1146, 879)),
  ('Info', 'Next', (1849, 892)),
  ('Up', None, (1501, 883)),
  ('Left', None, (1304, 1077)),
  ('Ok', None, (1505, 1074)),
  ('Right', None, (1703, 1080)),
  ('Down', None, (1499, 1269)),
  ('VolUp', 'Sys Volume Up', (1195, 1403)),
  ('Win', None, (1498, 1536)),
  ('VolDown', "Sys Volume Down", (1210, 1600)),
  ('ChanUp', 'Clem Vol Up', (1811, 1413)),
  ('ChanDown', 'Clem Vol Down', (1792, 1604)),
  ('Mute', 'Mute', (1502, 1785)),
  ('PVR', None, (1226, 1867)),
  ('DVD', "Print tags", (1771, 1872)),
  ('EPG', None, (1360, 2025)),
  ('Tuner', None, (1644, 2029)),
  ('1', '1 Stars', (1242, 2195)),
  ('2', None, (1504, 2197)),
  ('3', '1 Stars + Next', (1758, 2194)),
  ('4', '2 Stars', (1237, 2350)),
  ('5', None, (1504, 2350)),
  ('6', '2 Stars + Next', (1766, 2348)),
  ('7', '3 Stars', (1237, 2504)),
  ('8', None, (1503, 2504)),
  ('9', '3 Stars + Next', (1763, 2506)),
  ('*', '4 Stars', (1231, 2663)),
  ('0', None, (1506, 2664)),
  ('#', '4 Stars + Next', (1771, 2663)),
  ('Clear', '5 Stars', (1364, 2830)),
  ('Enter', '5 Stars + Next', (1637, 2831)),
  ('Red', None, (1202, 2982)),
  ('Green', None, (1394, 3041)),
  ('Blue', 'Rename file', (1794, 2977)),
  ('Yellow', None, (1607, 3037)),
  ('Subtitle', 'Pause', (1503, 3204)),
]

ELLIPSE_LIST = [
  ((1242, 2195), (1364, 2830)),

]

FONT_COLOR = 0,0,0
FONT_SIZE = 100

LINE_WIDTH = 10
LINE_GAP = 20
LINE_COLOR = 150,0,0

LEFT_START_POS = (150, 0)
RIGHT_START_POS = (2250, 0)

def main():
  # pprint.pprint(LABEL_LIST)
  # return

  im = Image.open("remote.jpg")

  fnt = ImageFont.truetype('/usr/share/fonts/truetype/freefont/FreeSansBold.ttf', FONT_SIZE)

  draw = ImageDraw.Draw(im)

  split_x = 1558
  left_pos = LEFT_START_POS
  right_pos = RIGHT_START_POS

  for button_name, label_text, button_pos in LABEL_LIST:
    if label_text is None:
      continue
    if button_pos[0] <= split_x:
      left_pos = adj_pos(draw, left_pos, button_pos, label_text, fnt)
      text_pos = left_pos
      line_start_pos = line_start_left(draw, text_pos, label_text, fnt)
    else:
      right_pos = adj_pos(draw, right_pos, button_pos, label_text, fnt)
      text_pos = right_pos
      line_start_pos = line_start_right(draw, text_pos, label_text, fnt)

    draw.text(text_pos, label_text, font=fnt, fill=FONT_COLOR)
    draw.line((line_start_pos, button_pos), fill=LINE_COLOR, width=LINE_WIDTH)

  draw.ellipse(pad_box(ELLIPSE_LIST[0], 100), fill=None, outline=0)


  # Pillow's drawing primitives don't do antialiasing, so we draw first on a
  # large image then downsample in order to get acceptable quality.
  im.thumbnail((500, 1000), resample=PIL.Image.LANCZOS)
  with open('remote-labels.png', 'wb') as f:
    im.save(f, "PNG")


def adj_pos(draw, pos_tup, button_pos, text, font):
  size_tup = draw.textsize(text, font=font)
  pos_tup = pos_tup[0], pos_tup[1] + size_tup[1]
  if pos_tup[1] < button_pos[1] - size_tup[1] / 2:
    pos_tup = pos_tup[0], button_pos[1] - size_tup[1] / 2
  return pos_tup

def pad_box(b, t):
  return (
    (b[0][0] - t, b[0][1] - t),
    (b[1][0] + t, b[1][1] + t)
  )


def line_start_left(draw, pos_tup, text, font):
  size_tup = draw.textsize(text, font=font)
  return pos_tup[0] + size_tup[0] + LINE_GAP, pos_tup[1] + size_tup[1] / 2

def line_start_right(draw, pos_tup, text, font):
  size_tup = draw.textsize(text, font=font)
  return pos_tup[0] - LINE_GAP, pos_tup[1] + size_tup[1] / 2

if __name__ == '__main__':
  main()
