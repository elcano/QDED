#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/highgui.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <map> 
#include <algorithm> 
#include <chrono> 
using namespace std;
using namespace std::chrono;
using namespace cv;
//Cone height ~ 18 inch
//Cone width ~ 11 inch
//Cone mid width ~ 4 inch

//If values have been read in by QDED this method marks the edges.
int findConeEdges(Mat image, std::vector<int> col, std::vector<int> row, std::vector<int> angle)
{
	double colNum = 0;
	double rowNum = 0;
	int maxVal = 0;
	int maxRow = image.rows;
	for (int i = 0; i < col.size(); i++)
	{
		colNum += col[i]; 
		rowNum += row[i];
		if (col[i] > maxVal)
		{
			maxVal = col[i];
		}
		if (row[i] < maxRow)
		{
			maxRow = row[i];
		}

	}
	colNum /= col.size();
	rowNum /= col.size();

	for (int i = 0; i < col.size(); i++)
	{
		image.at<Vec3b>(row[i], col[i])[2] = 255;
		image.at<Vec3b>(row[i], col[i])[1] = 255;
		image.at<Vec3b>(row[i], col[i])[0] = 254;

		//Code for angle
		//If the angle is 90 to 75 go up
		//checkAngle(image, col, row, angle, i, 90, 75);
		
	}
	return maxRow;

}

//Reads data from QDED files
//Takes in the file, image, a maxStr and vectors to store the data
void readFromFile(String file, Mat image, int maxStr, std::vector<int> str, std::vector<int> & col, std::vector<int> & row,std::vector<int> &angleVec)
{
	std::ifstream myfile;
	myfile.open(file);
	String line;
	int angle;
	double c = 0;
	double r = 0;
	double num = 0;
	int count = 0;
	if (myfile.is_open())
	{
		getline(myfile, line); // gets first line
		//getline(myfile, line); //gets header
		while (getline(myfile, line))
		{
			myfile >> angle; //angle
			myfile >> c; //column
			myfile >> r; //row
			myfile >> num; //str
			if (num < -maxStr || num > maxStr) //num > maxStr ||
			{
				//std::cout << num << std::endl;
				str.push_back(num);
				angleVec.push_back(angle);
				col.push_back(c);
				row.push_back(r);
				count++;
			}
		}
		myfile.close();
	}
	else
	{
		std::cout << "Unable to open file";
	}
	count = 0;
	bool testing = 0;

	if (testing == 1)
	{
		//Finding cones
		double colNum = 0;
		double rowNum = 0;
		for (int i = 0; i < col.size(); i++)
		{
			colNum += col[i];
			rowNum += row[i];
		}
		colNum /= col.size();
		rowNum /= col.size();
		image.at<Vec3b>(rowNum, colNum)[2] = 255;
		image.at<Vec3b>(rowNum+1, colNum)[2] = 255;
		image.at<Vec3b>(rowNum, colNum+1)[2] = 255;
		image.at<Vec3b>(rowNum-1, colNum)[2] = 255;
		image.at<Vec3b>(rowNum, colNum-1)[2] = 255;

	}
	else
	{
		/*
		for (int i = 0; i < column.size(); i++)
		{
			image.at<Vec3b>(row[i], column[i])[2] = 255;
			image.at<Vec3b>(row[i], column[i])[1] = 0;
			image.at<Vec3b>(row[i], column[i])[0] = 0;

		}
		*/
	}	
}

//Rotates an Image by an angle
void rotateImage(Mat & image, int angle)
{
	Point2f center((image.cols - 1) / 2.0F, (image.rows - 1) / 2.0F);
	Mat rot = getRotationMatrix2D(center, 90, 1);
	Rect2f bbox = RotatedRect(Point2f(), image.size(), 90).boundingRect2f();
	rot.at<double>(0, 2) += bbox.width / 2.0 - image.cols / 2.0;
	rot.at<double>(1, 2) += bbox.height / 2.0 - image.rows / 2.0;
	warpAffine(image, image, rot, bbox.size());
}

//Takes in an image and turns it an greyscale image where orange is white and not orange is black
// May need some fine tuning on the color.
Mat greyScaleOrange(Mat image)
{
	Mat imageHSV;
	cvtColor(image, imageHSV, COLOR_BGR2HSV);


	for (int row = 0; row < imageHSV.rows; row++)
	{
		for (int col = 0; col < imageHSV.cols; col++)
		{

			if (imageHSV.at<Vec3b>(row, col)[0] < 10 && imageHSV.at<Vec3b>(row, col)[0] > 0)
			{
				image.at<Vec3b>(row, col)[0] = 255;
				image.at<Vec3b>(row, col)[1] = 255;
				image.at<Vec3b>(row, col)[2] = 255;
			}
			else
			{
				image.at<Vec3b>(row, col)[0] = 255/179 * (179 - imageHSV.at<Vec3b>(row, col)[0]);
				image.at<Vec3b>(row, col)[1] = 255 / 179 * (179 - imageHSV.at<Vec3b>(row, col)[0]);
				image.at<Vec3b>(row, col)[2] = 255 / 179 * (179 - imageHSV.at<Vec3b>(row, col)[0]);
			}

		}
	}

	cvtColor(imageHSV, imageHSV, COLOR_HSV2BGR);
	return imageHSV;


}


//Finds the center of a cone
// Returns the point of which is the center of mass
Point findCenterCone(Mat image)
{
	Mat imageHSV;
	cvtColor(image, imageHSV, COLOR_BGR2HSV);
	Mat imgThresholded;
	//Look for an orange color
	inRange(imageHSV, Scalar(0, 150, 60), Scalar(22, 255, 255), imgThresholded);

	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));


	Mat res;
	bitwise_and(image, image, res, imgThresholded);

	Moments oMoments = moments(imgThresholded);

	double dM01 = oMoments.m01;
	double dM10 = oMoments.m10;
	double dArea = oMoments.m00;

	if (dArea > 10000)
	{
		//calculate the position of the object (in this case a cone);
		int posX = dM10 / dArea;
		int posY = dM01 / dArea;

		return Point(posX, posY);
	}

	return Point(0, 0);
}

void takeVideo()
{
		VideoCapture cap(0);
		Mat frame;

		//Mat frame;
		while (cap.isOpened())
		{
			//Add methods here for video
			cap >> frame;
			//Get the center
			Point pos = findCenterCone(frame);

			//Draw a circle around the center
			circle(frame, pos, 20, Scalar(0, 255, 0));

			//show image
			imshow("result", frame);



			waitKey(1);


		}
		cap.release();
}



int main()
{
	int debug = 0;
	int maxStr = 50;
	int maxRow = 0;
	std::vector<int> str;
	std::vector<int> row;
	std::vector<int> col;
	std::vector<int> angleVec;

	if (debug == -1)
	{
		// Does video
		takeVideo();
	}
	else if (debug == 0)
	{
		//Finds the center of a cone

		//Read in an image
		Mat image = imread("image.jpg");

		//Get the center
		Point pos = findCenterCone(image);

		//Draw a circle around the center
		circle(image, pos, 20, Scalar(0, 255, 0));

		//show image
		imshow("result", image);
	}
	else if (debug == 1)
	{
		//Gets the edges of a QDED image
		Mat image = imread("image.jpg");
		readFromFile("Lreadable.txt", image, maxStr, str, col, row, angleVec);
		readFromFile("Ureadable.txt", image, maxStr, str, col, row, angleVec);
		maxRow = findConeEdges(image, col, row, angleVec);
		imshow("result", image);
		waitKey(0);
	}




	return 0;
}



	//ALL of this code is unused but saved here for future use. This code either didn't work or didn't have use.

	/*vector<int> values(10000);

			// Generate Random values 
			auto f = []() -> int { return rand() % 10000; };

			// Fill up the vector 
			generate(values.begin(), values.end(), f);

			// Get starting timepoint 
			auto start = high_resolution_clock::now();


			cap >> frame;
			//Mat image = frame.clone();
			Mat dst;
			Size size(320, 240);
			resize(frame, dst, size);

			imwrite("image.jpg", dst);
			system("Run.bat");



			//findColor(frame);
			//Read in an image
			
			//Get the center
			//Point pos = findCenter(frame);
			//std::vector<std::pair<Point, Point>> lines;

			//Read the files
			readFromFile("Lreadable.txt", dst, maxStr, str, col, row, angleVec);
			readFromFile("Ureadable.txt", dst, maxStr, str, col, row, angleVec);
			maxRow = findCone(dst, col, row, angleVec);
			str.clear();
			col.clear();
			row.clear();
			angleVec.clear();
			
			imshow("frame", dst);
			waitKey(1);

			auto stop = high_resolution_clock::now();

			// Get duration. Substart timepoints to  
			// get durarion. To cast it to proper unit 
			// use duration cast method 
			auto duration = duration_cast<microseconds>(stop - start);

			cout << "Time taken by function: " << duration.count() << " microseconds" << endl;

	

	int findDistance(Point a, Point b)
	{
	return std::sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
	}

	bool checkWhite(Mat image, Point a, Point b)
	{
	double count = 0;
	double total = 0;
	for (int i = a.x; i < b.x; i++)
	{
	total++;
	if (image.at<Vec3b>(a.y, i)[0] == 255)
	{
	count++;
	}
	}
	if (count / total >= .5)
	{
	return true;
	}
	return false;
	}


	void drawLines(Mat image, std::vector<std::pair<Point, Point>> lines)
	{
	//WE have a bunch of lines, now we want to check where most of these lines are.
	//We need to find the center of each line and see if they match up

	std::vector<std::pair<Point, Point>> goodLines;
	std::vector<int> centers;
	int avg = 0;

	for (int i = 0; i < lines.size(); i++)
	{
	centers.push_back(findDistance(lines[i].first, lines[i].second) / 2);
	avg += findDistance(lines[i].first, lines[i].second) / 2;
	}
	avg /= lines.size();

	for (int i = 0; i < centers.size(); i++)
	{
	if (centers[i] < avg + 10 && centers[i] > avg - 10)
	{
	goodLines.push_back(lines[i]);

	}
	}
	}

	std::vector<std::pair<Point,Point>> findLine(Mat image, std::vector<int> col, std::vector<int> row)
	{
	//Searchs for lines
	Point current;
	Point next;

	Point start;
	Point end;

	std::vector<std::pair<Point,Point>> lines;

	for (int i = 0; i < col.size(); i++)
	{
	//Get first point
	current = Point(col[i], row[i]);
	start = current;
	for (int j = 0; j < col.size(); j++)
	{
	//Get next point
	next = Point(col[j], row[j]);
	//If with in
	if (findDistance(Point(0,current.y),Point(0,next.y)) <= 1)
	{

	current = next;
	}
	}
	end = current;
	if (checkWhite(image, start, end))
	{
	lines.push_back(std::make_pair(start, end));
	line(image, lines[i].first, lines[i].second, Scalar(0, 0, 0), 1, 8, 0);
	}

	}
	return lines;
	}

	void checkAngle(Mat image, std::vector<int> col, std::vector<int> row, std::vector<int> angle, int i, int low, int high)
	{
	if (angle[i] < 90 && angle[i] > 60 && row[i] > 5)
	{
	for (int j = 1; j < 5; j++)
	{
	image.at<Vec3b>(row[i], col[i]-j)[2] = 255;
	image.at<Vec3b>(row[i], col[i]-j)[1] = 0;
	image.at<Vec3b>(row[i], col[i]-j)[0] = 0;
	}

	}


	}

	void checkerBoard(Mat & image, std::vector<int> col, std::vector<int> row)
	{

	int number = 5;
	int * ptr;
	const int n = number * number;// keep this number * number so 5*5 = 25
	ptr = new int[n];
	int count = 0;
	int pointer = 0;

	int colNum = image.cols / number;
	int rowNum = image.rows / number;

	//Draws the checker board
	for (int i = 0; i < number; i++)
	{
	line(image, Point(i*colNum, 0), Point(i*colNum, image.rows), Scalar(0, 0, 0), 1, 8);
	line(image, Point(0, i*rowNum), Point(image.cols, i*rowNum), Scalar(0, 0, 0), 1, 8);

	}


	int temp = 0;
	int temp2 = 0;
	for (int i = 0; i < number; i++)
	{
	for (int j = 0; j < number; j++)
	{
	for (int k = 0; k < col.size(); k++)
	{
	if (col[k] > colNum*i && col[k] < colNum*(i + 1) && row[k] > rowNum*j && row[k] < rowNum*(j+1))
	{
	count++;
	}

	}
	//Prints out number to the checker board
	putText(image, std::to_string(count), Point(colNum*i, rowNum*(j+1)), 1, 1, Scalar(0, 0, 0), 1, 8);
	ptr[pointer] = count;
	pointer++;
	count = 0;
	}

	}
	}

	bool compareContourAreas(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2) {
	double i = fabs(contourArea(cv::Mat(contour1)));
	double j = fabs(contourArea(cv::Mat(contour2)));
	return (i < j);
	}

	RotatedRect findMarker(Mat image)
	{
	Mat Grey;
	cvtColor(image, Grey, COLOR_BGR2GRAY);
	GaussianBlur(Grey, Grey, Size(5, 5), 0);
	Mat Edge;
	Canny(Grey, Edge, 35, 125);

	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	findContours(Edge.clone(), contours, hierarchy,RETR_TREE, CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	std::sort(contours.begin(), contours.end(), compareContourAreas);

	// grab contours
	std::vector<cv::Point> biggestContour = contours[contours.size() - 1];
	std::vector<cv::Point> smallestContour = contours[0];
	minAreaRect(biggestContour);
	return minAreaRect(biggestContour);
	}




	float findDistance(int knownWidth,int focal, int perWidth)
	{
	return (knownWidth * focal) / perWidth;

	}

	float testingDistance(Mat image)
	{
	RotatedRect marker = findMarker(image);
	return (marker.size.height * 24) / 16;
	}

	void DrawRotatedRectangle(cv::Mat& image, RotatedRect rotatedRectangle)
	{
	cv::Scalar color = cv::Scalar(255.0, 255.0, 255.0); // white

	// Create the rotated rectange
	// We take the edges that OpenCV calculated for us
	cv::Point2f vertices2f[4];
	rotatedRectangle.points(vertices2f);

	// Convert them so we can use them in a fillConvexPoly
	cv::Point vertices[4];
	for (int i = 0; i < 4; ++i) {
	vertices[i] = vertices2f[i];
	}

	// Now we can fill the rotated rectangle with our specified color
	cv::fillConvexPoly(image,
	vertices,
	4,
	color);
	}

	void greyScaleColor(Mat image)
	{

	Mat imageHSV;
	cvtColor(image, imageHSV, COLOR_BGR2HSV);
	imshow("GreyScale Orange", imageHSV);
	waitKey(0);

	Mat HSV[3];
	split(imageHSV, HSV);
	//HSV[0] = 100;
	//HSV[1] = 100;
	HSV[2] = 255;
	std::vector<Mat> channels = { HSV[0],HSV[1],HSV[2] };
	merge(channels, imageHSV);
	imshow("GreyScale Orange", imageHSV);
	waitKey(0);
	cvtColor(imageHSV, imageHSV, COLOR_HSV2BGR);
	cvtColor(imageHSV, imageHSV, COLOR_RGB2GRAY);

	imshow("GreyScale Orange", imageHSV);
	waitKey(0);

	}



	Mat findColor(Mat image)
	{
	Mat imageHSV;
	cvtColor(image, imageHSV, COLOR_BGR2HSV);
	Mat imgThresholded;
	inRange(imageHSV, Scalar(0, 150, 60), Scalar(22, 255, 255), imgThresholded);

	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));


	Mat res;
	bitwise_and(image, image,res, imgThresholded);

	Mat matDst(Size(image.cols * 2, image.rows), image.type(), Scalar::all(0));
	Mat matRoi = matDst(Rect(0, 0, image.cols, image.rows));
	image.copyTo(matRoi);
	//
	matRoi = matDst(Rect(image.cols, 0, image.cols, image.rows));
	res.copyTo(matRoi);

	Moments oMoments = moments(imgThresholded);

	double dM01 = oMoments.m01;
	double dM10 = oMoments.m10;
	double dArea = oMoments.m00;

	if (dArea > 10000)
	{
	//calculate the position of the ball
	int posX = dM10 / dArea;
	int posY = dM01 / dArea;

	int iLastX = -1;
	int iLastY = -1;
	Mat imgLines = Mat::zeros(imgThresholded.size(), CV_8UC3);;
	if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0)
	{
	//Draw a red line from the previous point to the current point
	line(imgLines, Point(posX, posY), Point(iLastX, iLastY), Scalar(0, 0, 255), 2);
	}

	iLastX = posX;
	iLastY = posY;
	circle(matDst, Point(posX, posY), 20, Scalar(0, 255, 0));
	}

	return res;


	}

	void distanceTesting()
	{
	Mat image = imread("image.jpg");
	Mat res;
	res = findColor(image);
	RotatedRect marker = findMarker(res);
	float focal = testingDistance(res);
	float inches = findDistance(16, focal, marker.size.height);
	putText(res, std::to_string(inches), Point(10, 200), 1, 1, Scalar(255, 255, 255), 1, 8);
	//DrawRotatedRectangle(res, marker);
	imshow("image", res);
	waitKey(0);

	Mat image2 = imread("image4ft.jpg");
	Mat res2 = findColor(image2);

	marker = findMarker(res2);
	inches = findDistance(16, focal, marker.size.height);
	putText(res2, std::to_string(inches), Point(10, 200), 1, 1, Scalar(255, 255, 255), 1, 8);
	//DrawRotatedRectangle(res2, marker);
	imshow("image", res2);
	waitKey(0);

	}



	Point findClosest(int x, int y, std::vector<int> col, std::vector<int> row)
	{
	int closeX = INT_MAX;
	int temp = -1;
	for (int i = 0; i < col.size(); i++)
	{
	if (findDistance(Point(x, y), Point(col[i], row[i])) < closeX)
	{
	closeX = findDistance(Point(x, y), Point(col[i], row[i]));
	temp = i;
	}
	}
	return Point(col[temp], row[temp]);
	}

	double findShape(Mat image, Point center)
	{



	greyScaleOrange(image);

	//100 width
	//175 height
	int width = 0;
	int height = 0;

	double count = 0;
	double total = 0;

	for (int row = center.y - width; row < center.y + width; row++)
	{
	for (int col = center.x - height; col < center.x + height; col++)
	{
	if (image.at<Vec3b>(row, col)[0] == 255)
	{
	line(image, Point(col, row), Point(col, row), Scalar(0, 00, 255));
	count++;
	}
	total++;
	}
	}




	imshow("Shape",image);
	waitKey(1);
	return count / total;
	}

	void executeBatch(char* BatchFile)
	{
	std::string cmd(BatchFile);
	std::string expandCmd = '\"' + cmd + '\"';
	system(expandCmd.c_str());
	}


	*/


}
