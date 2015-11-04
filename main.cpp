#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <vector>
#include <string>


#include "../libs/log.h"
#include "../libs/parserxml.h"
#include "../libs/sockethandler.h"

//opencv
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
//

using namespace std;

cv::VideoCapture cap(0);
//Перехват изображения
class RobotCamera : private noncopyable {
public:

    static cv::Mat getImageFromCamera() {
        myLog << "get image from camera";

        if(cap.isOpened() == false) {
            myLog << "ERROR: can`t initialise the cam";
        }

        cv::Mat currentImage;
        for(int i = 0; i < 6; i++) {
            cap >> currentImage;
        }
        return currentImage;
    }

    static cv::Mat getImageFromFile() {
        using namespace cv;

        Mat src = imread("/home/vladimir-notebook/triangle");
        if (src.data == 0) {
            myLog << "can`t read image";
        }
        return src;
    }
};

enum class Shape {
    Square,
    Triangle,
    Circle,
    NoShape
};
//Распознавание образа
class ShapeDetector : private noncopyable {
public:
    static double threshold;

    static Shape recognizeShape(cv::Mat inputImage) {
        using namespace cv;

        Mat grayImage;

        cvtColor(inputImage, grayImage, CV_BGR2GRAY);
        GaussianBlur(grayImage, grayImage, Size(9, 9), 2, 2);

        if(findCircle(grayImage) == true) {
            debugShowImage(grayImage);
            return Shape::Circle;
        }

        Canny(grayImage, grayImage, threshold/3, threshold, 3);
        dilate(grayImage, grayImage, Mat(), Point(-1,-1));

        debugShowImage(grayImage);

        waitKey(0);

        vector<vector<Point> > contours;
        findContours(grayImage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

        vector<Point> approx;
        for(size_t i = 0; i < contours.size(); i++) {
            approxPolyDP(Mat(contours[i]), approx, fabs(contourArea(Mat(contours[i]), true))*0.001, true);

            myLog << fabs(contourArea(contours[i])) << " " << approx.size();


            if (fabs(contourArea(contours[i])) < 15000) {
                continue;
            }

            drawContours(inputImage, contours, i, Scalar(0, 0, 255), 10);
            debugShowImage(inputImage);
            waitKey(0);

            if (approx.size() == 3) {
                drawContours(inputImage, contours, i, Scalar(0, 0, 255), 10);
                myLog << approx.size() << " triangle ";

                debugShowImage(inputImage);
                return Shape::Triangle;
            }
            else if (approx.size() == 4) {
                drawContours(inputImage, contours, i, Scalar(0, 0, 255), 10);
                myLog << approx.size() << " square ";

                debugShowImage(inputImage);
                return Shape::Square;
            }
        }

        return Shape::NoShape;
    }

    static bool findCircle(cv::Mat &inputGrayImage) {
        using namespace cv;

        vector<Vec3f> circles;
        HoughCircles(inputGrayImage, circles, CV_HOUGH_GRADIENT, 1, inputGrayImage.rows / 8, threshold, 100, 50, 0);

        debugShowImage(inputGrayImage);

        for(size_t i = 0; i < circles.size(); i++) {
            Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);

            circle(inputGrayImage, center, radius, Scalar(0, 0, 255), 20, 8, 0);
            debugShowImage(inputGrayImage);
            myLog << "circle";
        }

        if(circles.size() > 0) {
            return true;
        } else {
            return false;
        }
    }

    static int debugShowImage(cv::Mat image) {
        using namespace cv;

        namedWindow("image", WINDOW_NORMAL);
        imshow("image", image);

        imwrite("test.jpg", image);

        uj8 bwaitKey(0);
        return 0;
    }
};
double ShapeDetector::threshold = 70;

class ComputerVisionClient : public QObject, private noncopyable {
    Q_OBJECT

    SocketHandler socketHandler;
    QString serverIp, serverPort;




public:
    ComputerVisionClient(QObject *parent = 0) : QObject(parent) {
        connect(&socketHandler, SIGNAL(isConnected()), this, SLOT(socketConnected()));
        connect(&socketHandler, SIGNAL(isDisconnected()), this, SLOT(socketDisconnected()), Qt::QueuedConnection);
        connect(&socketHandler, SIGNAL(packageReceived(QByteArray)), this, SLOT(commandsReseived(QByteArray)));

        ShapeDetector::threshold = 100;
    }

public slots:

    void start() {
        QHash <QString, QHash <QString, QString> > settings = ParserXML::loadXmlSettings(QDir::currentPath() + "/" + "../xml/settings.xml");

        if(settings.contains("server") == true) {
            QHash <QString, QString> serverSettings = settings["server"];
            if(serverSettings.contains("ip") == true
                    && serverSettings.contains("clients_port") == true) {

                serverIp = serverSettings["ip"];
                serverPort = serverSettings["clients_port"];
            } else {
                myLog << "ERROR: server settings should contain ip and clients_port";
                return;
            }
        } else {
            myLog << "ERROR: server settings not found";
            return;
        }

        serverIp = settings["server"]["ip"];
        serverPort = settings["server"]["clients_port"];

        myLog << "server settings ip=" << serverIp << " port=" << serverPort;

        myLog << "connect to server" << "\n";
        socketHandler.sConnect(serverIp, serverPort.toInt());
    }


private slots:

    void socketDisconnected() {
        myLog << "ERROR: connection was brocken try to reconnect";
        socketHandler.reconnect();
        myLog << "reconnect end";
    }

    void socketConnected() {
        myLog << "socket connected successful";
    }

    void commandsReseived(QByteArray commandPackage) {
        myLog << "command package reseived. size=" << commandPackage.size();
        QStringList commands = ParserXML::parseCmdPackage(commandPackage);

        myLog << "commandsReceived:";
        for(QString str : commands) {
            myLog << str;
        }

        for(QString currentCommand : commands) {
            QStringList parametres = currentCommand.split(" ", QString::SkipEmptyParts);

            if(parametres.length() == 0) {
                continue;
            }

            /* run command */
        }

        QString shape;
        switch (ShapeDetector::recognizeShape(RobotCamera::getImageFromFile))) {
        case Shape::Circle:
            shape = "круг";
            break;
        case Shape::Square:
            shape = "квадрат";
            break;
        case Shape::Triangle:
            shape = "треугольник";
            break;
        case Shape::NoShape:
            shape = "ничего";
            break;
        }
        myLog << "распознана фигура " << shape;

        sendElementsToServer(shape);
    }

    void sendElementsToServer(QString shape) {
        myLog << "generate elements";

        QHash <QString, QVariant> elements;
        elements["компьютерное зрение "] = shape;

        myLog << "generated elements:\n";
        for(QString currentElement : elements.keys()) {
            myLog << "elem=" << currentElement
                  << "value=" << elements[currentElement].toString();
        }

        myLog << "send package to planner";
        QByteArray elementsPackage = ParserXML::generateClientsPackage(elements);
        socketHandler.sendData(elementsPackage);
    }


};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ComputerVisionClient computerVisionClient;
    QTimer::singleShot(0, &computerVisionClient, SLOT(start()));

    return a.exec();
}

#include <main.moc>
