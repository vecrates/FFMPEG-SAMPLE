package cn.vecrates.ffmpeg.util;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.text.TextUtils;

import java.io.File;
import java.io.IOException;

public class UriUtil {

    public static boolean isUriExist(Context context, String uri) {
        return isUriExist(context, Uri.parse(uri));
    }

    public static boolean isUriExist(Context context, Uri uri) {
        if (context == null) {
            return false;
        }
        ParcelFileDescriptor pfd = null;
        ContentResolver cr = context.getContentResolver();
        try {
            pfd = cr.openFileDescriptor(uri, "r");
            if (pfd == null) {
                return false;
            }
        } catch (Exception e) {
            return false;
        } finally {
            try {
                if (pfd != null) {
                    pfd.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return true;
    }

    public static Uri queryPathUri(Context context, File file) {
        Cursor cursor = null;
        try {
            String filePath = file.getAbsolutePath();
            Uri queryUri = MediaStore.Files.getContentUri("external");
            String[] projection = new String[]{MediaStore.Images.Media._ID};
            String selection = MediaStore.Video.Media.DATA + "=? ";
            String[] args = new String[]{filePath};
            cursor = context.getContentResolver().query(queryUri,
                    projection,
                    selection,
                    args,
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                int id = cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.MediaColumns._ID));
                return Uri.withAppendedPath(queryUri, "" + id);
            }

            if (file.exists()) {
                ContentValues values = new ContentValues();
                values.put(MediaStore.Video.Media.DATA, filePath);
                return context.getContentResolver().insert(queryUri, values);
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    public static String queryUriPath(Context context, String uri) {
        return queryUriPath(context, Uri.parse(uri));
    }

    public static String queryUriPath(Context context, Uri uri) {
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(
                    uri,
                    new String[]{MediaStore.MediaColumns.DATA},
                    null,
                    null,
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                return cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DATA));
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    public static String queryUriMimeType(Context context, Uri uri) {
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(
                    uri,
                    new String[]{MediaStore.MediaColumns.MIME_TYPE},
                    null,
                    null,
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                return cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.MIME_TYPE));
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    public static boolean isUri(String string) {
        if (TextUtils.isEmpty(string)) {
            return false;
        }
        return string.startsWith("content:") || string.startsWith("file:");
    }

    public static boolean isFileUri(String string) {
        if (TextUtils.isEmpty(string)) {
            return false;
        }
        return string.startsWith("file:");
    }

    public static boolean isContentUri(String string) {
        if (TextUtils.isEmpty(string)) {
            return false;
        }
        return string.startsWith("content:");
    }

}
