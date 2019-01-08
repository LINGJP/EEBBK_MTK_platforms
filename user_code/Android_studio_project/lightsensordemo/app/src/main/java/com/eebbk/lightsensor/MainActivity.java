package com.eebbk.lightsensor;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.TextView;

import com.eebbk.common.ShellUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import static java.lang.Math.atan;
import static java.lang.Math.sqrt;
import static java.lang.Thread.sleep;

public class MainActivity extends AppCompatActivity implements SensorEventListener{
    private static final String CATCHANGEALSPS =  "cat /sys/class/misc/m_alsps_misc/changealsps";
    private static final String CATALS_3220 = "cat /sys/bus/platform/drivers/als_ps/als_rawdata";      //ap3220
    private static final String CATREALPS_3220 = "cat /sys/bus/platform/drivers/als_ps/ps_rawdata";    //ap3220
    private static final String CATALS = "cat /sys/bus/platform/drivers/als_ps/als";
    private static final String CATREALPS = "cat /sys/bus/platform/drivers/als_ps/ps";
    private static final String CATREALPS_PATCH = "/sys/bus/platform/drivers/als_ps/ps";
    public gsensorlister gsensortemp = new gsensorlister();
    private SensorManager sm;   //sensor manger

    boolean is3220 = false;
    private Sensor ligthSensor1 =null;    //光感
    private Sensor psensor1 =null;    //距离
    private TextView lightvalue;    //光感textviem
    private TextView psensorvalue;
    public psensorlister temp;

    public TextView psensorraw1;

    private TextView gsesort;
    private Sensor gsensor;
    private TextView alsraw;

    private TextView jiaodu;

    PowerManager powerManager = null;
    PowerManager.WakeLock wakeLock = null;


    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
           super.handleMessage(msg);
            String str = msg.obj.toString();
            str = str.replace("\n","");
            alsraw.setText("als_raw : " + str);
            psensorraw1.setText("ps_raw :" + msg.arg1);
        }
    };
    private float value[] = {2,3,4,8};//用来保存值
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        setTitle("传感器_LYQ");
        lightvalue = (TextView)findViewById(R.id.lightvalue);
        lightvalue.setText("LYQ!");
        lightvalue.setTextSize(30);
        psensorvalue = (TextView)findViewById(R.id.psensor);
        psensorvalue.setText("LYQ!");
        psensorvalue.setTextSize(30);

        psensorraw1= (TextView)findViewById(R.id.psensorraw);
        psensorraw1.setTextSize(30);

        gsesort = (TextView)findViewById(R.id.gsensorid);
        gsesort.setTextSize(30);

        alsraw = (TextView)findViewById(R.id.als_raw);
        alsraw.setTextSize(30);

        jiaodu = (TextView)findViewById(R.id.jiaodu);
        jiaodu.setTextSize(30);

        sm = (SensorManager) getSystemService(SENSOR_SERVICE);      //获取传感器服务，然后获取对应的具体sensor。
        //获取Sensor对象
        ligthSensor1 = sm.getDefaultSensor(Sensor.TYPE_LIGHT);     //应该是光感 //66是我自己添加的sensor type
        if(ligthSensor1==null)
            Log.e("lyq","light is null");
        psensor1 = sm.getDefaultSensor(Sensor.TYPE_PROXIMITY);
        gsensor = sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);   //lyqy应该是重力

      sm.registerListener(this,ligthSensor1,SensorManager.SENSOR_DELAY_NORMAL);    //注册光感的监听器

        temp = new psensorlister();
        sm.registerListener(temp,psensor1,SensorManager.SENSOR_DELAY_NORMAL);

        sm.registerListener(gsensortemp,gsensor,SensorManager.SENSOR_DELAY_NORMAL);

        /***一直唤醒屏幕**/
        powerManager = (PowerManager)this.getSystemService(this.POWER_SERVICE);
        wakeLock = this.powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Lock");

     /*判断 是哪一个IC*/
        File file = new File(CATREALPS_PATCH);
        if(file.exists())
        {
            is3220 = false;
            Log.e("lyq","is3220 is flase");
        }else
        {
            is3220 = true;
            Log.e("lyq","is3220 is true");
        }
     /*   try {

            String temp_string = "";
            temp_string = ShellUtil.exec(CATREALPS);
            if(temp_string.equals("")) {
                is3220 = true;          //另一种方法判断是哪一个IC
            }
        } catch (IOException e) {
            e.printStackTrace();
        }*/
        new Thread(new Runnable() {         //权限问题可能会导致PSENSOR无法正常回调
            private int rawtemp = 0;
            @Override
            public void run() {
                while (true) {
                    String str = "";
                    String str1 = "";
                   try {
                       if(is3220)
                       {
                           str = ShellUtil.exec(CATALS_3220);
                           str1 = ShellUtil.exec(CATREALPS_3220);
                       }
                       else
                       {
                           str = ShellUtil.exec(CATALS);
                           //str1 = ShellUtil.exec(CATREALPS);
                           str1 = readFileToString(new File(CATREALPS_PATCH));          // 另一种方式读取值
                       }
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    if (str != null) {
                        if(str.equals(""))
                        {
                            Log.e("lyq","error");
                        }
                        else
                        {
                        Message msg = Message.obtain();
                        msg.obj = str;  //+psensor raw data,自动回换行.
                        //mHandler.sendMessage(msg);
                        str1 = str1.replaceAll("\n","");
                        msg.arg1= Integer.parseInt(str1);
                        mHandler.sendMessage(msg);
                        }
                    }

                    try { //延时0.5S
                        sleep(500);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }//end of while
            }
        }).start();
    }
    private String readFileToString(File file) {    //读取一个文件的值，以字符串的形式返回
        BufferedReader bfr = null;
        StringBuffer sb = new StringBuffer();
        try {
            bfr = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = bfr.readLine()) != null) {
                sb.append(line);
            }
            return sb.toString();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (bfr != null) {
                try {
                    bfr.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return null;
    }
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        //必须要有这个方法
    }
    @Override
    protected void onResume() {         //create 方法后，紧接着就会调用这个方法。具体可以搜索activity的生命周期
        super.onResume();
        sm.registerListener(gsensortemp,gsensor,SensorManager.SENSOR_DELAY_NORMAL);
        sm.registerListener(temp,psensor1,SensorManager.SENSOR_DELAY_NORMAL);
        sm.registerListener(this,ligthSensor1,SensorManager.SENSOR_DELAY_NORMAL);
    }

    @Override
    protected void onStop() {
        super.onStop();
        sm.unregisterListener(gsensortemp,gsensor);
        sm.unregisterListener(temp,psensor1);
        sm.unregisterListener(this,ligthSensor1);
    }

    public void onSensorChanged(SensorEvent event) {
        Log.d("FINGHTING", "-------------------onSensorChanged------------------");
        //获取光线强度
       /// value[0] = event.values[0];
        Log.d("lyq",Log.getStackTraceString(new Throwable()));  //打印回调栈
        //value[1] = event.values[1];
        //value[2] = event.values[2];
       // value[3] = event.values[3];

        lightvalue.setText("光照强度 = "+ event.values[0] );
    }


    /*传感器监听器，必须implements SensorEventListener,而且要重写里面的两个方法。供上面注册使用。*/
    public class psensorlister implements SensorEventListener
    {
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            //必须要有这个方法
        }
        public void onSensorChanged(SensorEvent event) {        //有数据更新的时候，就会自动回调这个方法。
            Log.d("FINGHTING", "-------------------onSensorChanged------------------");
            //获取光线强度
            value[0] = event.values[0];
            value[1] = event.values[1];
            value[2] = event.values[2];
            // value[3] = event.values[3];

            psensorvalue.setText("距离是 = "+ value[0] +"   value[1] = " +value[1] + "   value[2] = " +value[2] );
        }
    }
    public class gsensorlister implements SensorEventListener
    {
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            //必须要有这个方法
        }
        public void onSensorChanged(SensorEvent event) {
            Log.d("FINGHTING", "-------------------onSensorChanged------------------");
            //获取gsensor数据
            value[0] = event.values[0];
            value[1] = event.values[1];
            value[2] = event.values[2];
            // value[3] = event.values[3]

            double g = sqrt(value[0] * value[0] + value[1] * value[1]);
            double cos = value[1] / g;
            if (cos > 1) {
                cos = 1;
            } else if (cos < -1) {
                cos = -1;
            }
            double rad = Math.acos(cos);
            if (value[0] < 0) {
                rad = 2 * Math.PI - rad;
            }

            double uiRot = getWindowManager().getDefaultDisplay().getRotation();

            double uiRad = Math.PI / 2 * 0;//uiRot;     //当uiRot一直为0的时候，计算出来的值才是在一个比较合适范围。但是自动旋转的时候，
                                                    //这个值是会动态变化的.
            rad -= uiRad;

            double degrees = (double) (180 * rad / Math.PI);

            gsesort.setText("当前角度: " + degrees +"  Rotation is "+ uiRot + "原始数据：X = "+ value[0] +"   Y = " +value[1] + "   Z = " +value[2] );
            degrees = Get_Angle(value[0],value[1],value[2],1);
           double y = Get_Angle(value[0],value[1],value[2],2);
           double z = Get_Angle(value[0],value[1],value[2],3);
           //  degrees = sqrt(degrees*degrees + y*y);
            jiaodu.setText("厂商与X轴角度：" + degrees +" 与Y："+y+" 与Z："+ z);

        }
    }

    public double Get_Angle(double x,double y,double z,int dir)
        {
            double temp;
            double res=0;
            switch(dir)
            {
                case 0://与自然Z轴的角度
                    temp=sqrt((x*x+y*y))/z;
                    res=atan(temp);
                    break;
                case 1://与自然X轴的角度
                    temp=x/sqrt((y*y+z*z));
                    res=atan(temp);
                    break;
                case 2://与自然Y轴的角度
                    temp=y/sqrt((x*x+z*z));
                    res=atan(temp);
                    break;
            }
            res = res*180/3.14;
            return res;//把弧度转换成角度
        }
    }
