// ============================================================
// isotool.cpp
// دوال التعامل مع ملفات ISO باستخدام libcdio
// ============================================================

#include "isotool.h"
#include "debughelper.h"

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// ============================================================
// 1. ثوابت (Constants)
// ============================================================

namespace {
const QString ERROR_FILE_NOT_FOUND = "الملف غير موجود: ";
const QString ERROR_OPEN_ISO = "فشل في فتح ملف ISO: ";
const QString ERROR_READ_CONTENTS = "فشل في قراءة محتويات ISO";
const QString ERROR_NO_FILES = "لم يتم استخراج أي ملف";
}

// ============================================================
// 2. دوال مساعدة (Helper Functions)
// ============================================================

static bool fileExists(const QString &filePath, QString &errorMessage)
{
    if (!QFileInfo(filePath).exists()) {
        errorMessage = ERROR_FILE_NOT_FOUND + filePath;
        return false;
    }
    return true;
}

static bool createOutputDir(const QString &outputPath, QString &errorMessage)
{
    if (!QDir().mkpath(outputPath)) {
        errorMessage = "فشل في إنشاء مجلد الوجهة: " + outputPath;
        return false;
    }
    return true;
}

// ============================================================
// 3. اختبار وجود المكتبة
// ============================================================

bool testLibCdio()
{
    debugFunctionStart("testLibCdio");

#ifdef CDIO_VERSION
    qDebug() << "✅ libcdio found! Version:" << CDIO_VERSION;
#else
    qDebug() << "❌ libcdio NOT found!";
    debugFunctionEnd("testLibCdio");
    return false;
#endif

    CdIo_t *cdio = cdio_open(NULL, DRIVER_DEVICE);
    if (cdio) {
        qDebug() << "✅ Device opened successfully!";
        cdio_destroy(cdio);
        debugFunctionEnd("testLibCdio");
        return true;
    } else {
        qDebug() << "⚠️ No device found, but library is linked.";
        debugFunctionEnd("testLibCdio");
        return true;
    }
}

// ============================================================
// 4. عرض محتويات ISO
// ============================================================

QStringList getIsoContents(const QString &isoPath, QString &errorMessage)
{
    debugFunctionStart("getIsoContents");
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("getIsoContents");
        return QStringList();
    }

    iso9660_t *p_iso = iso9660_open(isoPath.toUtf8().constData());
    if (!p_iso) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        debugError(errorMessage);
        debugFunctionEnd("getIsoContents");
        return QStringList();
    }

    CdioList_t *p_entlist = iso9660_ifs_readdir(p_iso, "/");
    QStringList files;

    if (p_entlist) {
        CdioListNode_t *p_entnode;
        _CDIO_LIST_FOREACH (p_entnode, p_entlist) {
            iso9660_stat_t *p_statbuf = (iso9660_stat_t *) _cdio_list_node_data(p_entnode);
            if (p_statbuf) {
                char filename[4096];
                iso9660_name_translate(p_statbuf->filename, filename);
                files << QString::fromUtf8(filename);
            }
        }
        _cdio_list_free(p_entlist, true, nullptr);
    }

    iso9660_close(p_iso);
    debugFileList("Files in ISO", files);
    debugFunctionEnd("getIsoContents");
    return files;
}

// ============================================================
// 5. استخراج ملف واحد من ISO
// ============================================================

bool extractFileFromIso(const QString &isoPath, const QString &fileName, const QString &outputPath, QString &errorMessage)
{
    debugFunctionStart("extractFileFromIso");
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    iso9660_t *p_iso = iso9660_open(isoPath.toUtf8().constData());
    if (!p_iso) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        debugError(errorMessage);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    iso9660_stat_t *p_statbuf = iso9660_ifs_stat_translate(p_iso, fileName.toUtf8().constData());
    if (!p_statbuf) {
        errorMessage = "الملف غير موجود في ISO: " + fileName;
        debugError(errorMessage);
        iso9660_close(p_iso);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    QString fullOutputPath = outputPath + "/" + fileName;
    FILE *p_outfd = fopen(fullOutputPath.toUtf8().constData(), "wb");
    if (!p_outfd) {
        errorMessage = "فشل في إنشاء الملف الناتج: " + fullOutputPath;
        debugError(errorMessage);
        free(p_statbuf);
        iso9660_close(p_iso);
        debugFunctionEnd("extractFileFromIso");
        return false;
    }

    const unsigned int i_blocks = (p_statbuf->size + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;
    for (unsigned int i = 0; i < i_blocks; i++) {
        char buf[ISO_BLOCKSIZE];
        const lsn_t lsn = p_statbuf->lsn + i;

        if (ISO_BLOCKSIZE != iso9660_iso_seek_read(p_iso, buf, lsn, 1)) {
            errorMessage = "خطأ في قراءة ملف ISO";
            debugError(errorMessage);
            fclose(p_outfd);
            free(p_statbuf);
            iso9660_close(p_iso);
            debugFunctionEnd("extractFileFromIso");
            return false;
        }
        fwrite(buf, ISO_BLOCKSIZE, 1, p_outfd);
    }

    fflush(p_outfd);
    fclose(p_outfd);
    free(p_statbuf);
    iso9660_close(p_iso);

    debugQuaZipResult("extractFileFromIso", true, fileName);
    debugFunctionEnd("extractFileFromIso");
    return true;
}

// ============================================================
// 6. استخراج كل الملفات من ISO
// ============================================================

bool extractAllFromIso(const QString &isoPath, const QString &outputPath, QString &errorMessage)
{
    debugFunctionStart("extractAllFromIso");
    errorMessage.clear();

    if (!fileExists(isoPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    if (!createOutputDir(outputPath, errorMessage)) {
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    iso9660_t *p_iso = iso9660_open(isoPath.toUtf8().constData());
    if (!p_iso) {
        errorMessage = ERROR_OPEN_ISO + isoPath;
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    CdioList_t *p_entlist = iso9660_ifs_readdir(p_iso, "/");
    if (!p_entlist) {
        errorMessage = ERROR_READ_CONTENTS;
        debugError(errorMessage);
        iso9660_close(p_iso);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    bool anySuccess = false;
    CdioListNode_t *p_entnode;
    _CDIO_LIST_FOREACH (p_entnode, p_entlist) {
        iso9660_stat_t *p_statbuf = (iso9660_stat_t *) _cdio_list_node_data(p_entnode);
        if (p_statbuf) {
            char filename[4096];
            iso9660_name_translate(p_statbuf->filename, filename);
            QString fileName = QString::fromUtf8(filename);

            if (fileName == "." || fileName == "..") continue;

            if (extractFileFromIso(isoPath, fileName, outputPath, errorMessage)) {
                anySuccess = true;
                qDebug() << "✅ Extracted:" << fileName;
            } else {
                qDebug() << "❌ Failed to extract:" << fileName;
            }
        }
    }

    _cdio_list_free(p_entlist, true, nullptr);
    iso9660_close(p_iso);

    if (!anySuccess) {
        errorMessage = ERROR_NO_FILES;
        debugError(errorMessage);
        debugFunctionEnd("extractAllFromIso");
        return false;
    }

    debugQuaZipResult("extractAllFromIso", true, "All files extracted");
    debugFunctionEnd("extractAllFromIso");
    return true;
}
