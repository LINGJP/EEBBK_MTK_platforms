#include <iostream>
using namespace std;
 
class Adder{
   public:
      // 构造函数
      Adder(int i = 0)
      {
        total = 9;
      }
      // 对外的接口
      void addNum(int number)
      {
          total += number;
      }
      // 对外的接口
      int getTotal()
      {
          return total;
      };
	protected:
	// 对外隐藏的数据
      int total;
};
class lyq: public Adder{
	public:
	int ss()
	{
	total=1;
	addNum(22);
	}
};
int main( )
{
   Adder a;
   lyq b;  
   a.addNum(10);
   a.addNum(20);
   a.addNum(30);
	b.ss(); 
   cout << "Total " << a.getTotal() <<endl;
   cout <<b.getTotal()<<endl;
   return 0;
}
