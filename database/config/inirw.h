#ifndef CONFIG_INIRW_H
#define CONFIG_INIRW_H

const int SIZE_FILENAME = 50;

class inirw
{
  private:
	inirw(const char *filename)
	{
		this->iniFileLoad(filename);
	};

	int iniFileLoad(const char *filename);
	static inirw *m_pInstance;

	class CGarboInirw //它的唯一工作就是在析构函数中删除CSingleton的实例
	{
	  public:
		~CGarboInirw()
		{
			if (inirw::m_pInstance)
			{
				delete inirw::m_pInstance;
			}
		}
	};
	static CGarboInirw Garbow; //定义一个静态成员变量，程序结束时，系统会自动调用它的析构函数

  public:
	static inirw *GetInstance(const char *filename); //获取实例

	~inirw() { iniFileFree(); };
	//加载ini文件至内存
	char gFilename[SIZE_FILENAME];
	char *gBuffer;
	int gBuflen;
	//释放ini文件所占资源
	void iniFileFree();
	int FindSection(const char *section, char **sect1, char **sect2, char **cont1, char **cont2, char **nextsect);
	//获取字符串，不带引号
	int iniGetString(const char *section, const char *key, char *value, int size, const char *defvalue);
	//获取整数值
	int iniGetInt(const char *section, const char *key, int defvalue);
	//获取浮点数
	double iniGetDouble(const char *section, const char *key, double defvalue);
	int iniGetValue(const char *section, const char *key, char *value, int maxlen, const char *defvalue);
	//设置字符串：若value为NULL，则删除该key所在行，包括注释
	int iniSetString(const char *section, const char *key, const char *value);
	//设置整数值：base取值10、16、8，分别表示10、16、8进制，缺省为10进制
	int iniSetInt(const char *section, const char *key, int value, int base = 10);
	//	int iniGetIP(const char *section, const char *key, BasicHashTable *hashtable, int size, const char *defvalue);
};
#endif
