#include "QtWebrtc.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QtWebrtc w;
	w.show();
	return a.exec();
}
