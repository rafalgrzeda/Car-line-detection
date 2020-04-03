#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv; 

Mat maskuj(Mat image, Mat src){
    // Pobranie wielkosci obrazu
    Size s = image.size();
    int height = s.height;
    int width = s.width;

    // Ustawienie parametrów geometrycznych ramki
    vector<Point> constSingle;

    //Nagranie 2
    constSingle.push_back(Point(0,height - 180));
    constSingle.push_back(Point(900,height - 180));
    constSingle.push_back(Point(600,400));
    constSingle.push_back(Point(260,400));
    constSingle.push_back(Point(0,height - 180));

    /*
    //Nagranie 3
    constSingle.push_back(Point(0,height));
    constSingle.push_back(Point(width,height));
    constSingle.push_back(Point(width,height-100));
    constSingle.push_back(Point(width-180,height-300));
    constSingle.push_back(Point(0,height-300));
    constSingle.push_back(Point(0,height));
    */
    vector<vector<Point> > array;
    array.push_back(constSingle);

    // stworzenie czarnego obrazu o wymiarach image
    Mat maska = Mat(height,width,CV_8UC1,Scalar(0,0,0));
    // dodanie białego trapeza (regionu zainteresowania)
    fillPoly(maska,array,255);
    // Maskowanie ( Region of Interest)
    Mat masked = image & maska;

    // Tylko do sprawdzenia (wyswietlania)
    Mat src2;
    src.copyTo(src2);
    fillPoly(src2,array,255);
    imshow("Maska1",src2);
    return masked;
}


Vec2d srednia(vector<Vec2d> linie){
    int ile = 0;
    double sum_nachylenie = 0;
    double sum_przeciecie = 0;
    double a = 0, b = 0;
    Vec2d l;
    for( size_t i = 0; i < linie.size(); i++ ){
        l = linie[i];
        sum_nachylenie = sum_nachylenie + l[0];
        sum_przeciecie = sum_przeciecie + l[1];
        ile++;
    }
    a = sum_nachylenie/ile;
    b = sum_przeciecie/ile;
    Vec2d rozw(a,b);
    return rozw;
}
Vec4d koordynaty(Vec2d linia, Mat image){
   double y1,y2,x1,x2;
   double a = linia[0];
   double b = linia[1];
   Size s = image.size();
   y1 = s.height;
   y2 = y1 *3/5;
   x1 = (y1 - b)/a;
   x2 = (y2 - b)/a;
   Vec4d output(x1,y1,x2,y2);
   //cout<<output<<endl;
   return output;
}
void detekcjaSkretow(Mat image, Mat src,double a_l, double a_p){

    Size s = image.size();
    int height = s.height;
    int width = s.width;

    if(a_l < -1){
        putText(src,"SKRET W LEWO",Point(width*2/5,height/2),FONT_HERSHEY_SIMPLEX,1,Scalar(0,255,0),4);

    }
    else if (a_p > 1) {
        putText(src,"SKRET W PRAWO",Point(width*2/5,height/2),FONT_HERSHEY_SIMPLEX,1,Scalar(0,255,0),4);
    }
    else{
        putText(src,"PROSTO",Point(width*2/5,height/2),FONT_HERSHEY_SIMPLEX,1,Scalar(0,255,0),4);
    }
}
vector<Vec2d> linieSrednie(vector<Vec4d> lines){
    vector<Vec2d> lewe_linie;
    vector<Vec2d> prawe_linie;
    Vec4d l;
    for( size_t i = 0; i < lines.size(); i++ )
    {
        l = lines[i];
        double angle = atan2(l[3]-l[1],l[2]-l[0])*180.0 / CV_PI;
        if(!(angle < 25 && angle > -25))                                                            // linie poziomie i zbliżone do nich nie są brane pod uwagę
        {

            // prosta typu y =ax + b
            double  a = (l[3] - l[1])/(l[2] - l[0]);
            double  b = l[3] - a * l[2];

            if(a < 0){
                lewe_linie.push_back(Vec2d(a,b));               // linie po lewej stornie jezdni (zbior)
            }
            else{
                prawe_linie.push_back(Vec2d(a,b));              // linie po prawej stornie jezdni(zbior)
            }
        }
    }
    // Tworzenie jednej linii prawej i lewej
    Vec2d lewa = srednia(lewe_linie);
    Vec2d prawa = srednia(prawe_linie);

    vector <Vec2d> output;
    output.push_back(lewa);
    output.push_back(prawa);

    return output;
}

int main()
{
    VideoCapture cap("/media/D4B9-8730/Projekt-WMAv2/Nagrania/nagranie2.mp4");
    if(!cap.isOpened()){
        cout<< "Nie mozna wyswietlic video"<<endl;
        return -1;
    }
    // Obraz zrodlowy
    Mat img_source;
    // Obraz do przeksztalcen
    Mat img;
    while(1){
        // Pobierz obraz (ramkę)
        cap >> img;

        // Jesli nie ma obrazu wyjdz z petli
        if(!cap.read(img))
            break;

        img_source = img;

        // W skali szarosci
        cvtColor(img_source,img,COLOR_RGB2GRAY);

        // Usuniecie szumów
        GaussianBlur(img,img,Size(5,5),0);

        // Krawedzie
        Canny(img,img,40,120);

        //Maska
        img = maskuj(img,img_source);

        // Wykrywanie linii
        vector<Vec4d> linie_zbior;
        HoughLinesP(img,linie_zbior,1,CV_PI/180,50,40,50);

        // Ze zbioru linii tworzymy 2 linie
       vector<Vec2d> linie_sr = linieSrednie(linie_zbior);

       Vec2d lewa = linie_sr[0];
       Vec2d prawa = linie_sr[1];

       // Detekcja skretow
       detekcjaSkretow(img,img_source,lewa[0],prawa[0]);

       // Przejscie na x1,y1,x2,y2
       Vec4d lewa_xy = koordynaty(lewa,img);
       Vec4d prawa_xy = koordynaty(prawa,img);

       // Wrysowywanie pasów do pierwotnego obrazka
       line( img_source, Point(int(lewa_xy[0]), int(lewa_xy[1])),Point(int(lewa_xy[2]), int(lewa_xy[3])), Scalar(0,255,0), 5);//, LINE_AA);
       line( img_source, Point(int(prawa_xy[0]), int(prawa_xy[1])),Point(int(prawa_xy[2]), int(prawa_xy[3])), Scalar(0,255,0), 5);//, LINE_AA);

        imshow("Window",img_source);
        waitKey(30);
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
