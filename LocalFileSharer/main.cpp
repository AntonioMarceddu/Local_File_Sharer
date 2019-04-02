#include "LocalFileSharer.hpp"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QString parameter;
	if (argc > 1)
	{
		parameter = argv[1];
	}
	else
	{
		parameter = "";
	}

	LocalFileSharer window(parameter);
	window.show();
	return app.exec();
}