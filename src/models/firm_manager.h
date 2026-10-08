#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class FirmManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentFirmName READ currentFirmName NOTIFY firmSwitched)
    Q_PROPERTY(QString currentFirmId READ currentFirmId NOTIFY firmSwitched)
    Q_PROPERTY(QVariantMap currentFirmInfo READ currentFirmInfo NOTIFY firmSwitched)
    Q_PROPERTY(QString activeFolder READ activeFolder WRITE setActiveFolder NOTIFY activeFolderChanged)

public:
    explicit FirmManager(QObject* parent = nullptr);

    QString currentFirmName() const;
    QString currentFirmId() const { return m_activeFirmId; }
    QVariantMap currentFirmInfo() const;
    QString activeFolder() const { return m_activeFolder; }
    void setActiveFolder(const QString& folder);

    Q_INVOKABLE QVariantList get_registered_firms();
    Q_INVOKABLE QVariantList scan_folder_for_firms(const QString& folderPath = "");
    Q_INVOKABLE bool switch_to_firm(const QString& firmId);
    Q_INVOKABLE bool prepare_firm_for_import(const QString& mdbFilePath, const QString& customFirmName = "", const QString& explicitFirmId = "");
    Q_INVOKABLE bool import_bahi_khata_firm(const QString& mdbFilePath, const QString& customFirmName = "");
    Q_INVOKABLE bool create_new_firm(const QVariantMap& firmInfo);
    Q_INVOKABLE QVariantMap get_current_firm_info();
    /// Registry entry (id/db_name/source_file/bahi_khata_file/...) for the
    /// active firm, or {} if none. Unlike currentFirmInfo() (company_info
    /// row), this carries file-mapping fields.
    QVariantMap currentFirmRegistryEntry() const;
    /// Persist the Bahi-Khata target file name (e.g. "Data.009") for a firm.
    bool setFirmBahiKhataFile(const QString& firmId, const QString& fileName);
    /// Derive "Data.NNN" from a sqlite db name / firm id
    /// ("..._data_004.db" -> "Data.004"). Returns "" when indeterminable.
    static QString deriveBahiKhataFileName(const QString& dbName, const QString& firmId);
    Q_INVOKABLE QString get_app_data_folder() const;    Q_INVOKABLE QString get_active_firm_folder();
    Q_INVOKABLE void set_active_firm_folder(const QString& folderPath);
    Q_INVOKABLE QString choose_firm_folder(const QString& currentFolder = "");
    Q_INVOKABLE void refresh_registry();

    static QString getFirmPeriod(const QString& dbPath);
    static QString formatDateToDisplay(const QString& rawDate);

signals:
    void firmSwitched(const QString& firmId, const QString& firmName);
    void activeFolderChanged();
    void registryUpdated();

private:
    QString m_activeFirmId;
    QString m_activeFolder;

    QString registryFilePath() const;
    void loadRegistry();
    void saveRegistry(const QVariantList& firms);
    static QString sanitizeSlug(const QString& name);
};
