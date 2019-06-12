import cv2
import sys
import os

if len(sys.argv) != 4:
	print("Usage: python {} hoge.wmv output_dir out_file_prefix".format(sys.argv[0]))
	exit(0)

cap = cv2.VideoCapture(sys.argv[1])
if cap.isOpened() == False:
	print("failed to open {}".format(sys.argv[1]))
	exit(0)

print("opened {}".format(sys.argv[1]))

dir = sys.argv[2]

try:
	print("create dir: {}".format(dir))
	os.mkdir(dir)
except FileExistsError:
	print("already exists {}".format(dir))
	pass

prefix = sys.argv[3]
index = 0

while True:
	ret, img = cap.read()
	if ret == False:
		break

	resized_img = cv2.resize(img, (512, 512))
	cv2.imwrite("{}/{}_{:06}.png".format(dir, prefix, index), resized_img)
	index += 1

	cv2.imshow("frame", resized_img)
	cv2.waitKey(1)


	
