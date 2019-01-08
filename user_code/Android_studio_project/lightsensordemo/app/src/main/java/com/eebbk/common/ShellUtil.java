
package com.eebbk.common;

import android.content.Context;
import android.os.PowerManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class ShellUtil {
    public static String exec(String command) throws IOException {
	    // start the ls command running
	    //String[] args =  new String[]{"sh", "-c", command};
	    Runtime runtime = Runtime.getRuntime();  
	    Process proc = runtime.exec(command);        //这句话就是shell与高级语言间的调用
		//如果有参数的话可以用另外一个被重载的exec方法
	        //实际上这样执行时启动了一个子进程,它没有父进程的控制台
	        //也就看不到输出,所以我们需要用输出流来得到shell执行后的输出
	        InputStream inputstream = proc.getInputStream();
	        InputStreamReader inputstreamreader = new InputStreamReader(inputstream);
	        BufferedReader bufferedreader = new BufferedReader(inputstreamreader);
	        // read the ls output
	        String line = "";
			StringBuilder sb = new StringBuilder(line);
	        while ((line = bufferedreader.readLine()) != null) {
	                sb.append(line);
	                sb.append('\n');
	        }
	        //tv.setText(sb.toString());
	        //使用exec执行不会等执行成功以后才返回,它会立即返回
	        //所以在某些情况下是很要命的(比如复制文件的时候)
	        //使用wairFor()可以等待命令执行完成以后才返回
	        try {
				int exit_value = proc.waitFor();
				String str_result = (exit_value == 0) ? "成功" : "失败" + command;
				Log.d("Anky", sb.toString() + str_result);
	        }
	        catch (Exception e) {
	            System.err.println(e);
	        }
	        finally {
				inputstream.close();
				inputstreamreader.close();
				bufferedreader.close();
				proc.destroy();
				//Log.d("Anky", sb.toString());
				return sb.toString();
	        }
	    }

	public static void reboot(Context context)
	{
		PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
		pm.reboot("hgc");
	}

	public static boolean isStringEmpty(String str)
	{
		if(str == null) return true;
		if("".equals(str)) return true;
		return false;
	}

	/** 执行 shell 命令之后返回 String 类型的结果 */
	public static String StringexecShellStr(String cmd)
	{
		String[] cmdStrings = new String[] {"sh", "-c", cmd};
		String retString = "";

		try
		{
			Process process = Runtime.getRuntime().exec(cmd);
			BufferedReader stdout =
					new BufferedReader(new InputStreamReader(
							process.getInputStream()), 7777);
			BufferedReader stderr =
					new BufferedReader(new InputStreamReader(
							process.getErrorStream()), 7777);

			String line = null;

			while ((null != (line = stdout.readLine()))
					|| (null != (line = stderr.readLine())))
			{
				if (false == isStringEmpty(line))
				{
					retString += line + "\n";
				}
			}

		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		return retString;
	}
}
