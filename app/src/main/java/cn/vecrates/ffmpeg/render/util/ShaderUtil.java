package cn.vecrates.ffmpeg.render.util;

import android.content.res.AssetManager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import cn.vecrates.ffmpeg.App;

public class ShaderUtil {

    public static String readByAssets(String path) {
        AssetManager manager = App.context.getAssets();
        String string = null;
        try {
            string = getStringByInputSteam(manager.open(path));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return string;
    }

    private static String getStringByInputSteam(InputStream is) {
        if (is == null) {
            return null;
        }
        InputStreamReader streamReader = null;
        StringBuilder sb = null;
        BufferedReader reader = null;
        try {
            streamReader = new InputStreamReader(is);
            reader = new BufferedReader(streamReader);
            String line;
            sb = new StringBuilder();
            while ((line = reader.readLine()) != null) {
                sb.append(line + "\n");
            }
            return sb.toString();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
            e.printStackTrace();
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }

                if (streamReader != null) {
                    streamReader.close();
                }
                if (is != null) {
                    is.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return null;
    }

}
