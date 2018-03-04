#!/usr/bin/env python

from tkinter import *
from PIL import Image, ImageTk

FILE_NAME = "remote.jpg"


def main():
  root = Tk()
  frame = Frame(root, bd=2, relief=SUNKEN)
  frame.grid_rowconfigure(0, weight=1)
  frame.grid_columnconfigure(0, weight=1)
  xscroll = Scrollbar(frame, orient=HORIZONTAL)
  xscroll.grid(row=1, column=0, sticky=E + W)
  yscroll = Scrollbar(frame)
  yscroll.grid(row=0, column=1, sticky=N + S)

  canvas = Canvas(
    frame, bd=0, xscrollcommand=xscroll.set, yscrollcommand=yscroll.set
  )
  canvas.grid(row=0, column=0, sticky=N + S + E + W)
  xscroll.config(command=canvas.xview)
  yscroll.config(command=canvas.yview)
  frame.pack(fill=BOTH, expand=1)

  img = ImageTk.PhotoImage(Image.open(FILE_NAME))
  canvas.create_image(0, 0, image=img, anchor="nw")
  canvas.config(scrollregion=canvas.bbox(ALL))

  def print_coords(event):
    print((event.x, event.y))

  canvas.bind("<Button 1>", print_coords)
  root.mainloop()


if __name__ == "__main__":
  main()
