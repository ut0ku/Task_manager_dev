#include "mainwindow.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>
#include <QMessageBox>
#include <QDate>
#include <QDebug>
#include <QFormLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDateEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isEnglish(false)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("task_manager.db");

    if (!db.open()) {
        QMessageBox::critical(this, tr("Error"), tr("Can't open database: ") + db.lastError().text());
        return;
    }

    executeSQL("CREATE TABLE IF NOT EXISTS Workspaces (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL);");
    executeSQL("CREATE TABLE IF NOT EXISTS Categories (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, workspace_id INTEGER, FOREIGN KEY(workspace_id) REFERENCES Workspaces(id));");
    executeSQL("CREATE TABLE IF NOT EXISTS Tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT NOT NULL, category_id INTEGER, difficulty TEXT, priority TEXT, status TEXT, deadline TEXT, FOREIGN KEY(category_id) REFERENCES Categories(id));");
    executeSQL("CREATE TABLE IF NOT EXISTS TaskTags (id INTEGER PRIMARY KEY AUTOINCREMENT, task_id INTEGER, tag TEXT, FOREIGN KEY(task_id) REFERENCES Tasks(id));");
    executeSQL("CREATE TABLE IF NOT EXISTS TaskHistory (id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT NOT NULL, category_id INTEGER, difficulty TEXT, priority TEXT, status TEXT, deadline TEXT, FOREIGN KEY(category_id) REFERENCES Categories(id));");

    loadWorkspaces();
    loadCategories();
    loadTasks();
    loadTaskHistory();

    setupUI();
    updateUI();
}

MainWindow::~MainWindow()
{
    qDeleteAll(workspaces);
    db.close();
}

void MainWindow::setupUI()
{
    mainWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(mainWidget);

    sidebar = new QScrollArea(this);
    sidebar->setFixedWidth(200);
    sidebar->setWidgetResizable(true);

    sidebarContent = new QWidget();
    sidebarLayout = new QVBoxLayout(sidebarContent);
    sidebarLayout->setAlignment(Qt::AlignTop);

    toggleSidebarButton = new QPushButton("☰", this);
    toggleSidebarButton->setFixedSize(30, 30);
    connect(toggleSidebarButton, &QPushButton::clicked, this, &MainWindow::toggleSidebar);

    addWorkspaceButton = new QPushButton(tr("Add Workspace"), this);
    connect(addWorkspaceButton, &QPushButton::clicked, this, &MainWindow::addWorkspace);

    sidebar->setFrameShape(QFrame::NoFrame);
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    sidebarLayout->addWidget(toggleSidebarButton);
    sidebarLayout->addWidget(addWorkspaceButton);
    toggleSidebarButton->raise();
    showWorkspaces();

    sidebar->setWidget(sidebarContent);

    workspaceView = new QWidget(this);
    workspaceLayout = new QVBoxLayout(workspaceView);

    currentWorkspaceLabel = new QLabel(tr("Select a workspace"), this);
    currentWorkspaceLabel->setAlignment(Qt::AlignCenter);

    addCategoryButton = new QPushButton(tr("Add Category"), this);
    addCategoryButton->setEnabled(false);
    connect(addCategoryButton, &QPushButton::clicked, this, &MainWindow::addCategory);

    categoriesScroll = new QScrollArea(this);
    categoriesScroll->setWidgetResizable(true);

    categoriesContent = new QWidget();
    categoriesLayout = new QVBoxLayout(categoriesContent);
    categoriesLayout->setAlignment(Qt::AlignTop);

    categoriesScroll->setWidget(categoriesContent);

    workspaceLayout->addWidget(currentWorkspaceLabel);
    workspaceLayout->addWidget(addCategoryButton);
    workspaceLayout->addWidget(categoriesScroll);

    QWidget *rightSidebar = new QWidget(this);
    QVBoxLayout *rightSidebarLayout = new QVBoxLayout(rightSidebar);
    rightSidebarLayout->setAlignment(Qt::AlignTop);

    historyButton = new QPushButton(tr("History"), this);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistory);

    notificationsButton = new QPushButton(tr("Notifications"), this);
    connect(notificationsButton, &QPushButton::clicked, this, &MainWindow::showNotifications);

    languageButton = new QPushButton(tr("English"), this);
    connect(languageButton, &QPushButton::clicked, this, &MainWindow::toggleLanguage);

    rightSidebarLayout->addWidget(historyButton);
    rightSidebarLayout->addWidget(notificationsButton);
    rightSidebarLayout->addWidget(languageButton);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(sidebar);
    contentLayout->addWidget(workspaceView);
    contentLayout->addWidget(rightSidebar);

    mainLayout->addLayout(contentLayout);
    setCentralWidget(mainWidget);

    resize(1000, 600);
    setWindowTitle(tr("Task Manager"));
}

void MainWindow::loadWorkspaces()
{
    QSqlQuery query("SELECT id, name FROM Workspaces;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        workspaces[name] = new Workspace(id, name);
    }
}

void MainWindow::loadCategories()
{
    QSqlQuery query("SELECT id, name, workspace_id FROM Categories;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        int workspaceId = query.value(2).toInt();

        for (auto it = workspaces.begin(); it != workspaces.end(); ++it) {
            if (it.value()->getId() == workspaceId) {
                it.value()->addCategory(name);
                break;
            }
        }
    }
}

void MainWindow::loadTasks()
{
    QSqlQuery query("SELECT id, description, category_id, difficulty, priority, status, deadline FROM Tasks;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString description = query.value(1).toString();
        int categoryId = query.value(2).toInt();
        QString difficulty = query.value(3).toString();
        QString priority = query.value(4).toString();
        QString status = query.value(5).toString();
        QString deadline = query.value(6).toString();

        QStringList tags;
        QSqlQuery tagQuery("SELECT tag FROM TaskTags WHERE task_id = " + QString::number(id) + ";");
        while (tagQuery.next()) {
            tags.append(tagQuery.value(0).toString());
        }

        for (auto workspaceIt = workspaces.begin(); workspaceIt != workspaces.end(); ++workspaceIt) {
            QMap<QString, Category*>& categories = workspaceIt.value()->getCategories();
            for (auto categoryIt = categories.begin(); categoryIt != categories.end(); ++categoryIt) {
                if (categoryIt.value()->getId() == categoryId) {
                    Task* task = new Task(id, description, categoryIt.value()->getName(), tags, difficulty, priority, status, deadline);
                    categoryIt.value()->addTask(task);
                    break;
                }
            }
        }
    }
}

void MainWindow::loadTaskHistory()
{
    QSqlQuery query("SELECT id, description, category_id, difficulty, priority, status, deadline FROM TaskHistory;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString description = query.value(1).toString();
        int categoryId = query.value(2).toInt();
        QString difficulty = query.value(3).toString();
        QString priority = query.value(4).toString();
        QString status = query.value(5).toString();
        QString deadline = query.value(6).toString();

        QStringList tags;
        QSqlQuery tagQuery("SELECT tag FROM TaskTags WHERE task_id = " + QString::number(id) + ";");
        while (tagQuery.next()) {
            tags.append(tagQuery.value(0).toString());
        }

        Task task(id, description, "", tags, difficulty, priority, status, deadline);
        taskHistory.append(task);
    }
}

void MainWindow::executeSQL(const QString& sql)
{
    QSqlQuery query;
    if (!query.exec(sql)) {
        qDebug() << "SQL error:" << query.lastError().text();
    }
}

void MainWindow::showWorkspaces()
{
    while (sidebarLayout->count() > 2) {
        QLayoutItem* item = sidebarLayout->takeAt(2);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    for (auto it = workspaces.begin(); it != workspaces.end(); ++it) {
        QPushButton *workspaceButton = new QPushButton(it.key(), sidebarContent);
        workspaceButton->setProperty("workspaceName", it.key());
        connect(workspaceButton, &QPushButton::clicked, this, &MainWindow::workspaceSelected);

        QPushButton *deleteButton = new QPushButton("×", sidebarContent);
        deleteButton->setFixedSize(20, 20);
        deleteButton->setProperty("workspaceName", it.key());
        connect(deleteButton, &QPushButton::clicked, this, &MainWindow::removeWorkspace);

        QHBoxLayout *workspaceLayout = new QHBoxLayout();
        workspaceLayout->addWidget(workspaceButton);
        workspaceLayout->addWidget(deleteButton);

        sidebarLayout->addLayout(workspaceLayout);
    }
}

void MainWindow::showCategories(const QString& workspaceName)
{
    QString cleanWorkspaceName = workspaceName;
    cleanWorkspaceName.replace(tr("Workspace: "), "");
    cleanWorkspaceName.replace(tr("Рабочее пространство: "), "");

    currentWorkspaceLabel->setText(tr("Workspace: %1").arg(cleanWorkspaceName));

    QLayoutItem* item;
    while ((item = categoriesLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }

    if (!workspaces.contains(workspaceName)) return;

    Workspace *workspace = workspaces[workspaceName];
    currentWorkspaceLabel->setText(tr("Workspace: %1").arg(workspaceName));
    addCategoryButton->setEnabled(true);

    QMap<QString, Category*>& categories = workspace->getCategories();
    for (auto it = categories.begin(); it != categories.end(); ++it) {
        QGroupBox *categoryGroup = new QGroupBox(it.key(), categoriesContent);
        QVBoxLayout *categoryLayout = new QVBoxLayout(categoryGroup);

        QPushButton *addTaskButton = new QPushButton(tr("Add Task"), categoryGroup);
        addTaskButton->setProperty("workspaceName", workspaceName);
        addTaskButton->setProperty("categoryName", it.key());
        connect(addTaskButton, &QPushButton::clicked, this, &MainWindow::addTask);

        QPushButton *deleteCategoryButton = new QPushButton(tr("Delete Category"), categoryGroup);
        deleteCategoryButton->setProperty("workspaceName", workspaceName);
        deleteCategoryButton->setProperty("categoryName", it.key());
        connect(deleteCategoryButton, &QPushButton::clicked, this, &MainWindow::removeCategory);

        QTableWidget *tasksTable = new QTableWidget(0, 6, categoryGroup);
        tasksTable->setHorizontalHeaderLabels(QStringList() << tr("Task") << tr("Deadline")
                                                            << tr("Status") << tr("Priority") << tr("Difficulty") << tr("Actions"));
        tasksTable->horizontalHeader()->setStretchLastSection(true);
        tasksTable->verticalHeader()->setVisible(false);
        tasksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tasksTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        tasksTable->setWordWrap(true);
        tasksTable->setColumnWidth(0, 300);

        QVector<Task*>& tasks = it.value()->getTasks();
        tasksTable->setRowCount(tasks.size());
        for (int i = 0; i < tasks.size(); ++i) {
            Task *task = tasks[i];

            QTableWidgetItem *taskItem = new QTableWidgetItem(task->getDescription());
            taskItem->setFlags(taskItem->flags() ^ Qt::ItemIsEditable);
            tasksTable->setItem(i, 0, taskItem);

            tasksTable->setItem(i, 1, new QTableWidgetItem(task->getDeadline()));
            tasksTable->setItem(i, 2, new QTableWidgetItem(task->getStatus()));
            tasksTable->setItem(i, 3, new QTableWidgetItem(task->getPriority()));
            tasksTable->setItem(i, 4, new QTableWidgetItem(task->getDifficulty()));

            QHBoxLayout *buttonLayout = new QHBoxLayout();

            QPushButton *pendingBtn = new QPushButton(tasksTable);
            pendingBtn->setIcon(QIcon(":/icons/pending.png"));
            pendingBtn->setToolTip("Pending");
            pendingBtn->setProperty("workspaceName", workspaceName);
            pendingBtn->setProperty("categoryName", it.key());
            pendingBtn->setProperty("taskDescription", task->getDescription());
            pendingBtn->setFixedSize(24, 24);
            connect(pendingBtn, &QPushButton::clicked, this, [this, workspaceName, it, task]() {
                changeTaskStatus(workspaceName, it.key(), task->getDescription(), "Pending");
            });

            QPushButton *inProgressBtn = new QPushButton(tasksTable);
            inProgressBtn->setIcon(QIcon(":/icons/inprogress.png"));
            inProgressBtn->setToolTip("In Progress");
            inProgressBtn->setProperty("workspaceName", workspaceName);
            inProgressBtn->setProperty("categoryName", it.key());
            inProgressBtn->setProperty("taskDescription", task->getDescription());
            inProgressBtn->setFixedSize(24, 24);
            connect(inProgressBtn, &QPushButton::clicked, this, [this, workspaceName, it, task]() {
                changeTaskStatus(workspaceName, it.key(), task->getDescription(), "In Progress");
            });

            QPushButton *completedBtn = new QPushButton(tasksTable);
            completedBtn->setIcon(QIcon(":/icons/completed.png"));
            completedBtn->setToolTip("Completed");
            completedBtn->setProperty("workspaceName", workspaceName);
            completedBtn->setProperty("categoryName", it.key());
            completedBtn->setProperty("taskDescription", task->getDescription());
            completedBtn->setFixedSize(24, 24);
            connect(completedBtn, &QPushButton::clicked, this, [this, workspaceName, it, task]() {
                changeTaskStatus(workspaceName, it.key(), task->getDescription(), "Completed");
            });

            QPushButton *deleteBtn = new QPushButton(tasksTable);
            deleteBtn->setIcon(QIcon(":/icons/delete.png"));
            deleteBtn->setToolTip("Delete");
            deleteBtn->setProperty("workspaceName", workspaceName);
            deleteBtn->setProperty("categoryName", it.key());
            deleteBtn->setProperty("taskDescription", task->getDescription());
            deleteBtn->setFixedSize(24, 24);
            connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::removeTask);

            buttonLayout->addWidget(pendingBtn);
            buttonLayout->addWidget(inProgressBtn);
            buttonLayout->addWidget(completedBtn);
            buttonLayout->addWidget(deleteBtn);
            buttonLayout->setSpacing(2);
            buttonLayout->setContentsMargins(0, 0, 0, 0);

            QWidget *buttonWidget = new QWidget(tasksTable);
            buttonWidget->setLayout(buttonLayout);
            tasksTable->setCellWidget(i, 5, buttonWidget);
        }

        tasksTable->resizeRowsToContents();

        categoryLayout->addWidget(addTaskButton);
        categoryLayout->addWidget(deleteCategoryButton);
        categoryLayout->addWidget(tasksTable);

        categoriesLayout->addWidget(categoryGroup);
    }
}


void MainWindow::toggleSidebar()
{
    if (sidebar->width() > 50) {

        int buttonY = toggleSidebarButton->y();

        sidebar->setFixedWidth(0);

        toggleSidebarButton->setParent(this);
        toggleSidebarButton->move(0, buttonY);
        toggleSidebarButton->setText("→");
        toggleSidebarButton->show();
    } else {
        sidebar->setFixedWidth(200);

        toggleSidebarButton->setParent(sidebarContent);
        sidebarLayout->insertWidget(0, toggleSidebarButton);
        toggleSidebarButton->setText("←");
    }
}

void MainWindow::addWorkspace()
{
    bool ok;
    QString workspaceName = QInputDialog::getText(this, tr("Add Workspace"),
                                                  tr("Workspace name:"), QLineEdit::Normal, "", &ok);
    if (ok && !workspaceName.isEmpty()) {
        QString sql = "INSERT INTO Workspaces (name) VALUES ('" + workspaceName + "');";
        executeSQL(sql);

        int id = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();
        workspaces[workspaceName] = new Workspace(id, workspaceName);

        showWorkspaces();
    }
}

void MainWindow::removeWorkspace()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    if (!workspaces.contains(workspaceName)) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Delete Workspace"),
                                  tr("Are you sure you want to delete workspace \"%1\" and ALL its content?").arg(workspaceName),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    int workspaceId = workspaces[workspaceName]->getId();

    QSqlDatabase::database().transaction();

    try {
        executeSQL(QString("DELETE FROM Tasks WHERE category_id IN "
                           "(SELECT id FROM Categories WHERE workspace_id = %1)").arg(workspaceId));

        executeSQL(QString("DELETE FROM Categories WHERE workspace_id = %1").arg(workspaceId));

        executeSQL(QString("DELETE FROM Workspaces WHERE id = %1").arg(workspaceId));

        QSqlDatabase::database().commit();
    } catch (...) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, tr("Error"), tr("Failed to delete workspace"));
        return;
    }

    Workspace* workspace = workspaces[workspaceName];

    auto categories = workspace->getCategories();
    for (auto it = categories.begin(); it != categories.end(); ++it) {
        Category* category = it.value();
        for (Task* task : category->getTasks()) {
            delete task;
        }
        delete category;
    }

    delete workspace;
    workspaces.remove(workspaceName);

    setupUI();
}

void MainWindow::workspaceSelected()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->text();

    workspaceName.replace(tr("Workspace: "), "");
    workspaceName.replace(tr("Рабочее пространство: "), "");

    showCategories(workspaceName);
}

void MainWindow::addCategory()
{
    QString workspaceName = currentWorkspaceLabel->text().replace(tr("Workspace: "), "");
    if (!workspaces.contains(workspaceName)) return;

    bool ok;
    QString categoryName = QInputDialog::getText(this, tr("Add Category"),
                                                 tr("Category name:"), QLineEdit::Normal, "", &ok);
    if (ok && !categoryName.isEmpty()) {
        QString sql = "INSERT INTO Categories (name, workspace_id) VALUES ('" + categoryName + "', " +
                      QString::number(workspaces[workspaceName]->getId()) + ");";
        executeSQL(sql);

        workspaces[workspaceName]->addCategory(categoryName);
        showCategories(workspaceName);
    }
}

void MainWindow::removeCategory()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    QString categoryName = button->property("categoryName").toString();

    if (!workspaces.contains(workspaceName)) return;

    Workspace *workspace = workspaces[workspaceName];
    if (!workspace->getCategories().contains(categoryName)) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Delete Category"),
                                  tr("Are you sure you want to delete category \"%1\"?").arg(categoryName),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QString sql = "DELETE FROM Categories WHERE id = " +
                      QString::number(workspace->getCategories()[categoryName]->getId()) + ";";
        executeSQL(sql);

        workspace->removeCategory(categoryName);
        showCategories(workspaceName);
    }
}

void MainWindow::addTask()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    QString categoryName = button->property("categoryName").toString();

    if (!workspaces.contains(workspaceName) || !workspaces[workspaceName]->getCategories().contains(categoryName)) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Task"));

    QFormLayout form(&dialog);

    QLineEdit *descriptionEdit = new QLineEdit(&dialog);
    QLineEdit *tagsEdit = new QLineEdit(&dialog);
    QComboBox *difficultyCombo = new QComboBox(&dialog);
    difficultyCombo->addItems(QStringList() << "Easy" << "Medium" << "Hard");
    QComboBox *priorityCombo = new QComboBox(&dialog);
    priorityCombo->addItems(QStringList() << "Low" << "Medium" << "High");
    QDateEdit *deadlineEdit = new QDateEdit(&dialog);
    deadlineEdit->setDisplayFormat("dd-MM-yyyy");
    deadlineEdit->setDate(QDate::currentDate());
    deadlineEdit->setCalendarPopup(true);

    form.addRow(tr("Description:"), descriptionEdit);
    form.addRow(tr("Tags (comma separated):"), tagsEdit);
    form.addRow(tr("Difficulty:"), difficultyCombo);
    form.addRow(tr("Priority:"), priorityCombo);
    form.addRow(tr("Deadline:"), deadlineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString description = descriptionEdit->text();
        QString tags = tagsEdit->text();
        QString difficulty = difficultyCombo->currentText();
        QString priority = priorityCombo->currentText();
        QString deadline = deadlineEdit->date().toString("dd-MM-yyyy");

        if (description.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Task description cannot be empty"));
            return;
        }

        QStringList tagList = tags.split(',', Qt::SkipEmptyParts);
        for (QString &tag : tagList) {
            tag = tag.trimmed();
        }

        Category *category = workspaces[workspaceName]->getCategories()[categoryName];

        QString sql = "INSERT INTO Tasks (description, category_id, difficulty, priority, status, deadline) VALUES ('" +
                      description + "', " + QString::number(category->getId()) + ", '" + difficulty + "', '" +
                      priority + "', 'Pending', '" + deadline + "');";
        executeSQL(sql);

        int taskId = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();

        for (const QString &tag : tagList) {
            sql = "INSERT INTO TaskTags (task_id, tag) VALUES (" + QString::number(taskId) + ", '" + tag + "');";
            executeSQL(sql);
        }

        Task *task = new Task(taskId, description, categoryName, tagList, difficulty, priority, "Pending", deadline);
        category->addTask(task);

        showCategories(workspaceName);
    }
}

void MainWindow::removeTask()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    QString categoryName = button->property("categoryName").toString();
    QString taskDescription = button->property("taskDescription").toString();

    if (!workspaces.contains(workspaceName) ||
        !workspaces[workspaceName]->getCategories().contains(categoryName)) {
        return;
    }

    Category *category = workspaces[workspaceName]->getCategories()[categoryName];
    Task *taskToDelete = nullptr;
    int taskIndex = -1;

    for (int i = 0; i < category->getTasks().size(); ++i) {
        if (category->getTasks()[i]->getDescription() == taskDescription) {
            taskToDelete = category->getTasks()[i];
            taskIndex = i;
            break;
        }
    }

    if (!taskToDelete) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Delete Task"),
                                  tr("Are you sure you want to delete task \"%1\"?").arg(taskDescription),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString sql = "DELETE FROM Tasks WHERE id = " + QString::number(taskToDelete->getId()) + ";";
    executeSQL(sql);

    sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(taskToDelete->getId()) + ";";
    executeSQL(sql);

    category->getTasks().remove(taskIndex);
    delete taskToDelete;

    showCategories(workspaceName);
}

void MainWindow::changeTaskStatus(const QString& workspaceName, const QString& categoryName,
                                  const QString& taskDescription, const QString& newStatus)
{
    if (!workspaces.contains(workspaceName)) {
        QMessageBox::warning(this, tr("Error"), tr("Workspace not found"));
        return;
    }

    Workspace *workspace = workspaces[workspaceName];
    if (!workspace->getCategories().contains(categoryName)) {
        QMessageBox::warning(this, tr("Error"), tr("Category not found"));
        return;
    }

    Category *category = workspace->getCategories()[categoryName];
    Task *taskToComplete = nullptr;
    int taskIndex = -1;

    for (int i = 0; i < category->getTasks().size(); ++i) {
        if (compareStringsIgnoreCase(category->getTasks()[i]->getDescription(), taskDescription)) {
            taskToComplete = category->getTasks()[i];
            taskIndex = i;
            break;
        }
    }

    if (!taskToComplete) {
        QMessageBox::warning(this, tr("Error"), tr("Task not found"));
        return;
    }

    QString oldStatus = taskToComplete->getStatus();

    QString sql = "UPDATE Tasks SET status = '" + newStatus + "' WHERE id = " +
                  QString::number(taskToComplete->getId()) + ";";
    executeSQL(sql);

    taskToComplete->setStatus(newStatus);

    if (newStatus.compare("Completed", Qt::CaseInsensitive) == 0 &&
        oldStatus.compare("Completed", Qt::CaseInsensitive) != 0) {

        sql = "INSERT INTO TaskHistory (description, category_id, difficulty, priority, status, deadline) "
              "VALUES ('" + taskToComplete->getDescription() + "', " + QString::number(category->getId()) +
              ", '" + taskToComplete->getDifficulty() + "', '" + taskToComplete->getPriority() + "', '" +
              taskToComplete->getStatus() + "', '" + taskToComplete->getDeadline() + "');";
        executeSQL(sql);

        int taskId = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();
        for (const auto& tag : taskToComplete->getTags()) {
            sql = "INSERT INTO TaskTags (task_id, tag) VALUES (" + QString::number(taskId) + ", '" + tag + "');";
            executeSQL(sql);
        }

        sql = "DELETE FROM Tasks WHERE id = " + QString::number(taskToComplete->getId()) + ";";
        executeSQL(sql);

        sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(taskToComplete->getId()) + ";";
        executeSQL(sql);

        taskHistory.append(*taskToComplete);

        Task* task = category->getTasks().takeAt(taskIndex);
        delete task;

        QMessageBox::information(this, tr("Task Completed"),
                                 tr("Task \"%1\" has been moved to history").arg(taskDescription));
    } else {
        QMessageBox::information(this, tr("Status Changed"),
                                 tr("Status for task \"%1\" has been updated").arg(taskDescription));
    }

    showCategories(workspaceName);
}

void MainWindow::showHistory()
{
    QDialog historyDialog(this);
    historyDialog.setWindowTitle(tr("Task History"));
    historyDialog.resize(800, 600);

    QVBoxLayout layout(&historyDialog);

    QTableWidget *historyTable = new QTableWidget(0, 6, &historyDialog);
    historyTable->setHorizontalHeaderLabels(QStringList() << tr("Task") << tr("Category") << tr("Deadline")
                                                          << tr("Status") << tr("Priority") << tr("Difficulty"));
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->verticalHeader()->setVisible(false);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    historyTable->setRowCount(taskHistory.size());
    for (int i = 0; i < taskHistory.size(); ++i) {
        const Task &task = taskHistory[i];
        historyTable->setItem(i, 0, new QTableWidgetItem(task.getDescription()));
        historyTable->setItem(i, 1, new QTableWidgetItem(task.getCategory()));
        historyTable->setItem(i, 2, new QTableWidgetItem(task.getDeadline()));
        historyTable->setItem(i, 3, new QTableWidgetItem(task.getStatus()));
        historyTable->setItem(i, 4, new QTableWidgetItem(task.getPriority()));
        historyTable->setItem(i, 5, new QTableWidgetItem(task.getDifficulty()));
    }

    QPushButton *restoreButton = new QPushButton(tr("Restore Task"), &historyDialog);
    QPushButton *deleteButton = new QPushButton(tr("Delete Task"), &historyDialog);
    QPushButton *closeButton = new QPushButton(tr("Close"), &historyDialog);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(restoreButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(closeButton);

    layout.addWidget(historyTable);
    layout.addLayout(buttonLayout);

    connect(restoreButton, &QPushButton::clicked, [this, &historyDialog]() {
        restoreTaskFromHistory();
        historyDialog.accept();
    });

    connect(deleteButton, &QPushButton::clicked, [this, &historyDialog]() {
        deleteTaskFromHistory();
        historyDialog.accept();
    });

    connect(closeButton, &QPushButton::clicked, &historyDialog, &QDialog::accept);

    historyDialog.exec();
}

void MainWindow::restoreTaskFromHistory()
{
    bool ok;
    QString taskDescription = QInputDialog::getText(this, tr("Restore Task"),
                                                    tr("Enter task description to restore:"),
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || taskDescription.isEmpty()) return;

    QString workspaceName = QInputDialog::getText(this, tr("Restore Task"),
                                                  tr("Enter workspace name:"),
                                                  QLineEdit::Normal, "", &ok);
    if (!ok || workspaceName.isEmpty()) return;

    QString categoryName = QInputDialog::getText(this, tr("Restore Task"),
                                                 tr("Enter category name:"),
                                                 QLineEdit::Normal, "", &ok);
    if (!ok || categoryName.isEmpty()) return;

    for (auto it = taskHistory.begin(); it != taskHistory.end(); ++it) {
        if (compareStringsIgnoreCase(it->getDescription(), taskDescription)) {
            if (!workspaces.contains(workspaceName)) {
                QMessageBox::warning(this, tr("Error"), tr("Workspace not found"));
                return;
            }

            Workspace *workspace = workspaces[workspaceName];
            if (!workspace->getCategories().contains(categoryName)) {
                QMessageBox::warning(this, tr("Error"), tr("Category not found in workspace"));
                return;
            }

            Category *category = workspace->getCategories()[categoryName];

            QString sql = "INSERT INTO Tasks (description, category_id, difficulty, priority, status, deadline) "
                          "VALUES ('" + it->getDescription() + "', " + QString::number(category->getId()) +
                          ", '" + it->getDifficulty() + "', '" + it->getPriority() + "', '" +
                          it->getStatus() + "', '" + it->getDeadline() + "');";
            executeSQL(sql);

            int taskId = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();

            for (const auto& tag : it->getTags()) {
                sql = "INSERT INTO TaskTags (task_id, tag) VALUES (" + QString::number(taskId) + ", '" + tag + "');";
                executeSQL(sql);
            }

            sql = "DELETE FROM TaskHistory WHERE id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            Task *task = new Task(taskId, it->getDescription(), categoryName,
                                  it->getTags(), it->getDifficulty(),
                                  it->getPriority(), it->getStatus(),
                                  it->getDeadline());
            category->addTask(task);

            taskHistory.erase(it);

            QMessageBox::information(this, tr("Task Restored"),
                                     tr("Task \"%1\" has been restored").arg(taskDescription));
            showCategories(workspaceName);
            return;
        }
    }

    QMessageBox::warning(this, tr("Error"), tr("Task not found in history"));
}

void MainWindow::deleteTaskFromHistory()
{
    bool ok;
    QString taskDescription = QInputDialog::getText(this, tr("Delete Task"),
                                                    tr("Enter task description to delete:"),
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || taskDescription.isEmpty()) return;

    for (auto it = taskHistory.begin(); it != taskHistory.end(); ++it) {
        if (compareStringsIgnoreCase(it->getDescription(), taskDescription)) {
            QString sql = "DELETE FROM TaskHistory WHERE id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            taskHistory.erase(it);

            QMessageBox::information(this, tr("Task Deleted"),
                                     tr("Task \"%1\" has been permanently deleted").arg(taskDescription));
            return;
        }
    }

    QMessageBox::warning(this, tr("Error"), tr("Task not found in history"));
}

void MainWindow::showNotifications()
{
    checkDeadlines();

    QDialog notificationsDialog(this);
    notificationsDialog.setWindowTitle(tr("Notifications"));

    QVBoxLayout layout(&notificationsDialog);

    QVector<Notification> unviewedNotifications;
    for (const Notification &n : notifications) {
        if (!n.isViewed()) {
            unviewedNotifications.append(n);
        }
    }

    if (unviewedNotifications.isEmpty()) {
        QLabel *noNotificationsLabel = new QLabel(tr("No new notifications"), &notificationsDialog);
        layout.addWidget(noNotificationsLabel);
    } else {
        QListWidget *notificationsList = new QListWidget(&notificationsDialog);
        for (const Notification &notification : unviewedNotifications) {
            notificationsList->addItem(notification.getMessage());
        }
        layout.addWidget(notificationsList);
    }

    QPushButton *clearButton = new QPushButton(tr("Clear Notifications"), &notificationsDialog);
    QPushButton *closeButton = new QPushButton(tr("Close"), &notificationsDialog);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(closeButton);

    layout.addLayout(buttonLayout);

    connect(clearButton, &QPushButton::clicked, [this, &notificationsDialog]() {
        clearNotifications();
        notificationsDialog.accept();
    });

    connect(closeButton, &QPushButton::clicked, &notificationsDialog, &QDialog::accept);

    notificationsDialog.exec();
}

void MainWindow::checkDeadlines()
{
    QDate currentDate = QDate::currentDate();

    for (auto workspaceIt = workspaces.begin(); workspaceIt != workspaces.end(); ++workspaceIt) {
        QMap<QString, Category*>& categories = workspaceIt.value()->getCategories();
        for (auto categoryIt = categories.begin(); categoryIt != categories.end(); ++categoryIt) {
            QVector<Task*>& tasks = categoryIt.value()->getTasks();
            for (Task *task : tasks) {
                QString taskDeadline = task->getDeadline();
                if (!taskDeadline.isEmpty()) {
                    QDate deadlineDate = QDate::fromString(taskDeadline, "dd-MM-yyyy");
                    if (deadlineDate.isValid() && deadlineDate == currentDate) {
                        bool exists = false;
                        for (const Notification &n : notifications) {
                            if (n.getTaskDescription() == task->getDescription() &&
                                n.getDeadline() == taskDeadline) {
                                exists = true;
                                break;
                            }
                        }

                        if (!exists) {
                            notifications.append(Notification(task->getDescription(), taskDeadline));
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::clearNotifications()
{
    for (Notification &n : notifications) {
        n.markAsViewed();
    }
}

void MainWindow::toggleLanguage()
{
    QApplication::removeTranslator(&translator);

    if (isEnglish) {
        if (translator.load(":/translations/taskmanager_ru.qm")) {
            QApplication::installTranslator(&translator);
        }
    } else {
        if (translator.load(":/translations/taskmanager_en.qm")) {
            QApplication::installTranslator(&translator);
        }
    }

    isEnglish = !isEnglish;
    languageButton->setText(isEnglish ? tr("Русский") : tr("English"));
    updateUI();
}


void MainWindow::updateUI()
{
    setWindowTitle(tr("Task Manager"));
    addWorkspaceButton->setText(tr("Add Workspace"));
    historyButton->setText(tr("History"));
    notificationsButton->setText(tr("Notifications"));
    addCategoryButton->setText(tr("Add Category"));
    languageButton->setText(isEnglish ? tr("Русский") : tr("English"));

    if (!currentWorkspaceLabel->text().isEmpty()) {
        QString cleanName = currentWorkspaceLabel->text();
        cleanName.replace(tr("Workspace: "), "");
        cleanName.replace(tr("Рабочее пространство: "), "");
        currentWorkspaceLabel->setText(tr("Workspace: %1").arg(cleanName));
    }

    showWorkspaces();
    if (!currentWorkspaceLabel->text().isEmpty() &&
        !currentWorkspaceLabel->text().contains(tr("Select a workspace"))) {
        QString cleanName = currentWorkspaceLabel->text();
        cleanName.replace(tr("Workspace: "), "");
        cleanName.replace(tr("Рабочее пространство: "), "");
        showCategories(cleanName);
    }
}

QString MainWindow::toLowerCase(const QString& str) const
{
    return str.toLower();
}

bool MainWindow::compareStringsIgnoreCase(const QString& a, const QString& b) const
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}
