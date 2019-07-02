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

	size = img.shape[0:2]
	max_dim = max(size)
	min_dim = min(size)
	unuse_width = int((max_dim-min_dim)/2)
	if size[0] < size[1]:
		# 横長
		top = 0
		bottom = min_dim
		left = int(unuse_width)
		right = int(max_dim-unuse_width)
		roi = img[top:bottom, left:right]
		cv2.imwrite("{}/{}_{:06}.png".format(dir, prefix, index), roi)
		cv2.imshow("frame", cv2.resize(roi, (512, 512)))
	else:
		# 縦長
		top = unuse_width
		bottom = int(max_dim-unuse_width)
		right = min_dim
		left = 0
		cv2.imwrite("{}/{}_{:06}.png".format(dir, prefix, index), roi)
		cv2.imshow("frame", cv2.resize(roi, (512, 512)))

	index += 1
	cv2.waitKey(1)


	
