#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QInputDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QAction>
#include <QTranslator>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDate>
#include <QApplication>

class Notification {
public:
    Notification(const QString& taskDesc, const QString& deadlineDate)
        : taskDescription(taskDesc), deadline(deadlineDate) {
        message = QObject::tr("Attention! Deadline for task \"%1\" expires: %2").arg(taskDesc).arg(deadlineDate);
    }

    QString getMessage() const { return message; }
    QString getTaskDescription() const { return taskDescription; }
    QString getDeadline() const { return deadline; }

private:
    QString message;
    QString taskDescription;
    QString deadline;
};

class Task {
public:
    Task(int id, const QString& desc, const QString& cat, const QStringList& tg,
         const QString& diff = "Medium", const QString& prio = "Medium",
         const QString& stat = "Pending", const QString& dl = "")
        : id(id), description(desc), category(cat), tags(tg),
        difficulty(diff), priority(prio), status(stat), deadline(dl) {}

    int getId() const { return id; }
    QString getDescription() const { return description; }
    QString getCategory() const { return category; }
    QStringList getTags() const { return tags; }
    QString getDifficulty() const { return difficulty; }
    QString getPriority() const { return priority; }
    QString getStatus() const { return status; }
    QString getDeadline() const { return deadline; }
    QString getWorkspace() const { return workspace; }

    void setDifficulty(const QString& diff) { difficulty = diff; }
    void setPriority(const QString& prio) { priority = prio; }
    void setStatus(const QString& stat) { status = stat; }
    void setDeadline(const QString& dl) { deadline = dl; }

    int daysUntilDeadline() const {
        if (deadline.isEmpty()) {
            return -1;
        }
        QDate deadlineDate = QDate::fromString(deadline, "dd-MM-yyyy");
        if (!deadlineDate.isValid()) {
            return -1;
        }
        QDate currentDate = QDate::currentDate();
        return currentDate.daysTo(deadlineDate);
    }

private:
    int id;
    QString description;
    QString category;
    QStringList tags;
    QString difficulty;
    QString priority;
    QString status;
    QString deadline;
    QString workspace;
};

class Category {
public:
    Category(int id, const QString& name) : id(id), name(name) {}

    int getId() const { return id; }
    void addTask(Task* task) { tasks.append(task); }
    QString getName() const { return name; }
    QVector<Task*>& getTasks() { return tasks; }

    void removeTask(const QString& taskDesc) {
        for (int i = 0; i < tasks.size(); ++i) {
            if (tasks[i]->getDescription() == taskDesc) {
                delete tasks[i];
                tasks.remove(i);
                break;
            }
        }
    }

private:
    int id;
    QString name;
    QVector<Task*> tasks;
};

class Workspace {
public:
    Workspace(int id, const QString& name) : id(id), name(name) {}

    int getId() const { return id; }

    void addCategory(const QString& categoryName) {
        categories[categoryName] = new Category(categories.size() + 1, categoryName);
    }

    QMap<QString, Category*>& getCategories() { return categories; }
    QString getName() const { return name; }

    void removeCategory(const QString& categoryName) {
        if (categories.contains(categoryName)) {
            delete categories[categoryName];
            categories.remove(categoryName);
        }
    }

private:
    int id;
    QString name;
    QMap<QString, Category*> categories;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleSidebar();
    void addWorkspace();
    void removeWorkspace();
    void workspaceSelected();
    void addCategory();
    void removeCategory();
    void addTask();
    void removeTask();
    void changeTaskStatus(const QString& workspaceName, const QString& categoryName,
                          const QString& taskDescription, const QString& newStatus);
    void showHistory();
    void restoreTaskFromHistory();
    void deleteTaskFromHistory();
    void showNotifications();
    void clearNotifications();
    void toggleLanguage();

private:
    void refreshEntireUI();
    void setupUI();
    void loadWorkspaces();
    void loadCategories();
    void loadTasks();
    void loadTaskHistory();
    void executeSQL(const QString& sql);
    void checkDeadlines();
    void showWorkspaces();
    void showCategories(const QString& workspaceName);
    void showTasks(const QString& workspaceName, const QString& categoryName);
    void updateUI();
    QString toLowerCase(const QString& str) const;
    bool compareStringsIgnoreCase(const QString& a, const QString& b) const;

    QSqlDatabase db;
    QVector<Notification> notifications;
    QMap<QString, Workspace*> workspaces;
    QVector<Task> taskHistory;
    QTranslator translator;
    bool isEnglish;

    // UI Elements
    QWidget *mainWidget;
    QVBoxLayout *mainLayout;
    QScrollArea *sidebar;
    QWidget *sidebarContent;
    QVBoxLayout *sidebarLayout;
    QPushButton *toggleSidebarButton;
    QPushButton *addWorkspaceButton;
    QPushButton *historyButton;
    QPushButton *notificationsButton;
    QPushButton *languageButton;
    QWidget *workspaceView;
    QVBoxLayout *workspaceLayout;
    QLabel *currentWorkspaceLabel;
    QPushButton *addCategoryButton;
    QScrollArea *categoriesScroll;
    QWidget *categoriesContent;
    QVBoxLayout *categoriesLayout;
};

#endif // MAINWINDOW_H
