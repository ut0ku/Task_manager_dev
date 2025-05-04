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

    // Create tables if they don't exist
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
    // Main widget and layout
    mainWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(mainWidget);

    // Sidebar
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

    sidebar->setFrameShape(QFrame::NoFrame); // Убираем границу
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    sidebarLayout->addWidget(toggleSidebarButton);
    sidebarLayout->addWidget(addWorkspaceButton);
    toggleSidebarButton->raise(); // Поднимаем кнопку поверх других виджетов
    // Show workspaces
    showWorkspaces();

    sidebar->setWidget(sidebarContent);

    // Workspace view area
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

    // Right sidebar
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

    // Main layout
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
    // Clear existing workspace buttons (preserve first two items)
    while (sidebarLayout->count() > 2) {
        QLayoutItem* item = sidebarLayout->takeAt(2);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Add workspace buttons
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
    // Clear existing categories
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

        QTableWidget *tasksTable = new QTableWidget(0, 5, categoryGroup);
        tasksTable->setHorizontalHeaderLabels(QStringList() << tr("Task") << tr("Deadline") << tr("Status") << tr("Priority") << tr("Difficulty"));
        tasksTable->horizontalHeader()->setStretchLastSection(true);
        tasksTable->verticalHeader()->setVisible(false);
        tasksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tasksTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        // Add tasks to table
        QVector<Task*>& tasks = it.value()->getTasks();
        tasksTable->setRowCount(tasks.size());
        for (int i = 0; i < tasks.size(); ++i) {
            Task *task = tasks[i];
            tasksTable->setItem(i, 0, new QTableWidgetItem(task->getDescription()));
            tasksTable->setItem(i, 1, new QTableWidgetItem(task->getDeadline()));
            tasksTable->setItem(i, 2, new QTableWidgetItem(task->getStatus()));
            tasksTable->setItem(i, 3, new QTableWidgetItem(task->getPriority()));
            tasksTable->setItem(i, 4, new QTableWidgetItem(task->getDifficulty()));

            // Add delete button
            QPushButton *deleteButton = new QPushButton(tr("Delete"), tasksTable);
            deleteButton->setProperty("workspaceName", workspaceName);
            deleteButton->setProperty("categoryName", it.key());
            deleteButton->setProperty("taskDescription", task->getDescription());
            connect(deleteButton, &QPushButton::clicked, this, &MainWindow::removeTask);

            tasksTable->setCellWidget(i, 5, deleteButton);
        }

        categoryLayout->addWidget(addTaskButton);
        categoryLayout->addWidget(deleteCategoryButton);
        categoryLayout->addWidget(tasksTable);

        categoriesLayout->addWidget(categoryGroup);

    }
}

void MainWindow::toggleSidebar()
{
    if (sidebar->width() > 50) {
        // Сохраняем текущую позицию кнопки
        int buttonY = toggleSidebarButton->y();

        // Полностью скрываем sidebar
        sidebar->setFixedWidth(0);

        // Делаем кнопку плавающей
        toggleSidebarButton->setParent(this);
        toggleSidebarButton->move(0, buttonY);
        toggleSidebarButton->setText("→");
        toggleSidebarButton->show();
    } else {
        // Возвращаем sidebar
        sidebar->setFixedWidth(200);

        // Возвращаем кнопку в sidebar
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

    // Получаем ID workspace для каскадного удаления
    int workspaceId = workspaces[workspaceName]->getId();

    // Удаляем из базы данных (с каскадным удалением)
    QSqlDatabase::database().transaction(); // Начинаем транзакцию

    try {
        // 1. Удаляем задачи и их теги
        executeSQL(QString("DELETE FROM Tasks WHERE category_id IN "
                           "(SELECT id FROM Categories WHERE workspace_id = %1)").arg(workspaceId));

        // 2. Удаляем категории
        executeSQL(QString("DELETE FROM Categories WHERE workspace_id = %1").arg(workspaceId));

        // 3. Удаляем само workspace
        executeSQL(QString("DELETE FROM Workspaces WHERE id = %1").arg(workspaceId));

        QSqlDatabase::database().commit(); // Подтверждаем изменения
    } catch (...) {
        QSqlDatabase::database().rollback(); // Откатываем при ошибке
        QMessageBox::critical(this, tr("Error"), tr("Failed to delete workspace"));
        return;
    }

    // Удаляем из памяти
    Workspace* workspace = workspaces[workspaceName];

    // Сначала удаляем все категории и задачи
    auto categories = workspace->getCategories();
    for (auto it = categories.begin(); it != categories.end(); ++it) {
        Category* category = it.value();
        for (Task* task : category->getTasks()) {
            delete task;
        }
        delete category;
    }

    // Затем само workspace
    delete workspace;
    workspaces.remove(workspaceName);

    // Обновляем интерфейс
    showWorkspaces();
    currentWorkspaceLabel->setText(tr("Select a workspace"));
    addCategoryButton->setEnabled(false);

    // Очищаем виджет категорий
    QLayoutItem* item;
    while ((item = categoriesLayout->takeAt(0))) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void MainWindow::workspaceSelected()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
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

    // Удаляем из базы данных
    QString sql = "DELETE FROM Tasks WHERE id = " + QString::number(taskToDelete->getId()) + ";";
    executeSQL(sql);

    sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(taskToDelete->getId()) + ";";
    executeSQL(sql);

    // Удаляем из памяти
    category->getTasks().remove(taskIndex);
    delete taskToDelete;

    showCategories(workspaceName);
}

void MainWindow::changeTaskStatus(const QString& workspaceName, const QString& categoryName,
                                  const QString& taskDescription, const QString& newStatus)
{
    if (!workspaces.contains(workspaceName)) return;

    Workspace *workspace = workspaces[workspaceName];
    if (!workspace->getCategories().contains(categoryName)) return;

    Category *category = workspace->getCategories()[categoryName];
    for (Task *task : category->getTasks()) {
        if (compareStringsIgnoreCase(task->getDescription(), taskDescription)) {
            QString sql = "UPDATE Tasks SET status = '" + newStatus + "' WHERE id = " +
                          QString::number(task->getId()) + ";";
            executeSQL(sql);

            QString oldStatus = task->getStatus();
            task->setStatus(newStatus);

            if (newStatus.compare("Completed", Qt::CaseInsensitive) == 0 &&
                oldStatus.compare("Completed", Qt::CaseInsensitive) != 0) {
                // Move task to history
                sql = "INSERT INTO TaskHistory (description, category_id, difficulty, priority, status, deadline) "
                      "VALUES ('" + task->getDescription() + "', " + QString::number(category->getId()) +
                      ", '" + task->getDifficulty() + "', '" + task->getPriority() + "', '" +
                      task->getStatus() + "', '" + task->getDeadline() + "');";
                executeSQL(sql);

                int taskId = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();
                for (const auto& tag : task->getTags()) {
                    sql = "INSERT INTO TaskTags (task_id, tag) VALUES (" + QString::number(taskId) + ", '" + tag + "');";
                    executeSQL(sql);
                }

                // Delete from active tasks
                sql = "DELETE FROM Tasks WHERE id = " + QString::number(task->getId()) + ";";
                executeSQL(sql);

                sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(task->getId()) + ";";
                executeSQL(sql);

                // Update in-memory data
                taskHistory.append(*task);
                category->removeTask(taskDescription);
                delete task;

                QMessageBox::information(this, tr("Task Completed"),
                                         tr("Task \"%1\" has been moved to history").arg(taskDescription));
            } else {
                QMessageBox::information(this, tr("Status Changed"),
                                         tr("Status for task \"%1\" has been updated").arg(taskDescription));
            }

            showCategories(workspaceName);
            return;
        }
    }

    QMessageBox::warning(this, tr("Error"), tr("Task not found"));
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

            // Remove from history
            sql = "DELETE FROM TaskHistory WHERE id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            // Create new task and add to category
            Task *task = new Task(taskId, it->getDescription(), categoryName,
                                  it->getTags(), it->getDifficulty(),
                                  it->getPriority(), it->getStatus(),
                                  it->getDeadline());
            category->addTask(task);

            // Remove from history in memory
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

    if (notifications.isEmpty()) {
        QLabel *noNotificationsLabel = new QLabel(tr("No new notifications"), &notificationsDialog);
        layout.addWidget(noNotificationsLabel);
    } else {
        QListWidget *notificationsList = new QListWidget(&notificationsDialog);
        for (const Notification &notification : notifications) {
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
    notifications.clear();
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
                        notifications.append(Notification(task->getDescription(), taskDeadline));
                    }
                }
            }
        }
    }
}

void MainWindow::clearNotifications()
{
    notifications.clear();
    // Обновляем базу данных, если уведомления там хранятся
    executeSQL("DELETE FROM Notifications WHERE viewed = 1"); // Пример
}

void MainWindow::toggleLanguage()
{
    isEnglish = !isEnglish;

    if (isEnglish) {
        if (translator.load(":/translations/taskmanager_en.qm")) {
            QApplication::instance()->installTranslator(&translator);
        }
        languageButton->setText(tr("Русский"));
    } else {
        QApplication::instance()->removeTranslator(&translator);
        languageButton->setText(tr("English"));
    }

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

    if (currentWorkspaceLabel->text() != tr("Select a workspace")) {
        QString workspaceName = currentWorkspaceLabel->text().replace(tr("Workspace: "), "");
        currentWorkspaceLabel->setText(tr("Workspace: %1").arg(workspaceName));
    } else {
        currentWorkspaceLabel->setText(tr("Select a workspace"));
    }

    showWorkspaces();
    if (!currentWorkspaceLabel->text().contains(tr("Select a workspace"))) {
        QString workspaceName = currentWorkspaceLabel->text().replace(tr("Workspace: "), "");
        showCategories(workspaceName);
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
