// ============================================================
// extractthread.h
// خيط منفصل لاستخراج الملفات (يمنع تجميد الواجهة)
// ============================================================

#ifndef EXTRACTTHREAD_H
#define EXTRACTTHREAD_H

#include <QThread>
#include <QString>

// ============================================================
// كلاس ExtractThread
// ============================================================

class ExtractThread : public QThread
{
    Q_OBJECT

public:
    // ─── المُنشئ ───
    explicit ExtractThread(QObject *parent = nullptr);

    // ─── تمرير البيانات إلى الخيط ───
    void setData(const QString &filePath, const QString &outputPath);

signals:
    // ─── إشارة لتحديث التقدم ───
    void progressUpdated(int current, int total);

    // ─── إشارة عند الانتهاء ───
    void finished(bool success, const QString &message);

protected:
    // ─── دالة التشغيل الرئيسية (تُنفذ في الخيط المنفصل) ───
    void run() override;

private:
    // ─── بيانات الاستخراج ───
    QString m_filePath;
    QString m_outputPath;
};

#endif // EXTRACTTHREAD_H
