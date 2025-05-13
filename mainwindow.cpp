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
#include <QEvent>

static QString translate(const char* text) {
    return QCoreApplication::translate("MainWindow", text);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isEnglish(false)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("task_manager.db");

    if (!db.open()) {
        QMessageBox::critical(this, translate("Error"), translate("Can't open database: ") + db.lastError().text());
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

QString MainWindow::translate(const QString& text) const {
    static const QMap<QString, QString> translations = {
        // Основное окно
        {"Менеджер задач", "Task Manager"},
        {"Добавить рабочее пространство", "Add Workspace"},
        {"Добавить категорию", "Add Category"},
        {"История", "History"},
        {"Уведомления", "Notifications"},
        {"Выберите рабочее пространство", "Select a workspace"},
        {"Рабочее пространство: %1", "Workspace: %1"},
        {"Русский", "Russian"},
        {"English", "English"},

        // Диалог добавления рабочего пространства
        {"Имя рабочего пространства:", "Workspace name:"},

        // Диалог добавления категории
        {"Имя категории:", "Category name:"},

        // Диалог создания задачи
        {"Создать задачу", "Create Task"},
        {"Название задачи:", "Task name:"},
        {"Описание:", "Description:"},
        {"Тэги (через запятую):", "Tags (comma separated):"},
        {"Сложность:", "Difficulty:"},
        {"Лёгкая", "Easy"},
        {"Средняя", "Medium"},
        {"Сложная", "Hard"},
        {"Приоритет:", "Priority:"},
        {"Низкий", "Low"},
        {"Средний", "Medium"},
        {"Высокий", "High"},
        {"Дата выполнения:", "Due date:"},
        {"Создать", "Create"},
        {"Отмена", "Cancel"},

        // Кнопки действий с задачами
        {"В ожидании", "Pending"},
        {"В процессе", "In Progress"},
        {"Завершено", "Completed"},
        {"Удалить", "Delete"},
        {"Действия", "Actions"},

        // Диалог истории
        {"Восстановить задачу", "Restore Task"},
        {"Удалить задачу", "Delete Task"},
        {"Закрыть", "Close"},
        {"Задача", "Task"},
        {"Категория", "Category"},
        {"Срок", "Deadline"},
        {"Статус", "Status"},
        {"Приоритет", "Priority"},
        {"Сложность", "Difficulty"},

        // Диалог уведомлений
        {"Нет новых уведомлений", "No new notifications"},
        {"Очистить уведомления", "Clear notifications"},
        {"Внимание! Срок выполнения задачи \"%1\" истекает: %2",
         "Attention! Deadline for task \"%1\" expires: %2"},

        // Сообщения об ошибках
        {"Ошибка", "Error"},
        {"Название задачи не может быть пустым", "Task name cannot be empty"},
        {"Описание задачи не может быть пустым", "Task description cannot be empty"},

        // Сообщения подтверждения
        {"Удалить рабочее пространство", "Delete Workspace"},
        {"Вы уверены, что хотите удалить рабочее пространство \"%1\" и ВСЕ его содержимое?",
         "Are you sure you want to delete workspace \"%1\" and ALL its content?"},
        {"Удалить категорию", "Delete Category"},
        {"Вы уверены, что хотите удалить категорию \"%1\"?",
         "Are you sure you want to delete category \"%1\"?"},
        {"Удалить задачу", "Delete Task"},
        {"Вы уверены, что хотите удалить задачу \"%1\"?",
         "Are you sure you want to delete task \"%1\"?"},
        {"Задача завершена", "Task Completed"},
        {"Задача \"%1\" перемещена в историю", "Task \"%1\" has been moved to history"},
        {"Статус изменен", "Status Changed"},
        {"Статус задачи \"%1\" обновлен", "Status for task \"%1\" has been updated"},

        // Восстановление из истории
        {"Введите описание задачи для восстановления:", "Enter task description to restore:"},
        {"Введите имя рабочего пространства:", "Enter workspace name:"},
        {"Введите имя категории:", "Enter category name:"},
        {"Задача не найдена в истории", "Task not found in history"},
        {"Задача восстановлена", "Task restored"},
        {"Задача удалена", "Task deleted"}
    };

    return isEnglish ? translations.value(text, text) : text;
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

    addWorkspaceButton = new QPushButton(translate("Add Workspace"), this);
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

    currentWorkspaceLabel = new QLabel(translate("Select a workspace"), this);
    currentWorkspaceLabel->setAlignment(Qt::AlignCenter);

    addCategoryButton = new QPushButton(translate("Add Category"), this);
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

    historyButton = new QPushButton(translate("History"), this);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistory);

    notificationsButton = new QPushButton(translate("Notifications"), this);
    connect(notificationsButton, &QPushButton::clicked, this, &MainWindow::showNotifications);

    languageButton = new QPushButton(translate("English"), this);
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
    setWindowTitle(translate("Task Manager"));
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

void MainWindow::showCategories(const QString& workspaceName) {
    currentWorkspaceLabel->setText(translate("Рабочее пространство: %1").arg(workspaceName));
    addCategoryButton->setEnabled(true);

    QLayoutItem* item;
    while ((item = categoriesLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }

    if (!workspaces.contains(workspaceName)) return;

    Workspace *workspace = workspaces[workspaceName];
    for (auto it = workspace->getCategories().begin(); it != workspace->getCategories().end(); ++it) {
        QGroupBox *group = new QGroupBox(it.key(), categoriesContent);
        QVBoxLayout *layout = new QVBoxLayout(group);

        QPushButton *addTaskBtn = new QPushButton(translate("Создать задачу"), group);
        addTaskBtn->setProperty("workspaceName", workspaceName);
        addTaskBtn->setProperty("categoryName", it.key());
        connect(addTaskBtn, &QPushButton::clicked, this, &MainWindow::addTask);

        QPushButton *deleteBtn = new QPushButton(translate("Удалить категорию"), group);
        deleteBtn->setProperty("workspaceName", workspaceName);
        deleteBtn->setProperty("categoryName", it.key());
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::removeCategory);

        QTableWidget *table = new QTableWidget(0, 6, group);
        table->setHorizontalHeaderLabels({
            translate("Задача"), translate("Срок"), translate("Статус"),
            translate("Приоритет"), translate("Сложность"), translate("Действия")
        });

        int row = 0;
        for (Task *task : it.value()->getTasks()) {
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(task->getDescription()));
            table->setItem(row, 1, new QTableWidgetItem(task->getDeadline()));

            QString status = task->getStatus();
            if (isEnglish) {
                if (status == "В ожидании") status = "Pending";
                else if (status == "В процессе") status = "In Progress";
                else if (status == "Завершено") status = "Completed";
            }
            table->setItem(row, 2, new QTableWidgetItem(status));

            table->setItem(row, 3, new QTableWidgetItem(task->getPriority()));
            table->setItem(row, 4, new QTableWidgetItem(task->getDifficulty()));

            QWidget *actions = new QWidget(table);
            QHBoxLayout *actionsLayout = new QHBoxLayout(actions);

            QPushButton *pendingBtn = new QPushButton(actions);
            pendingBtn->setIcon(QIcon(":/icons/pending.png"));
            pendingBtn->setToolTip(translate("В ожидании"));
            pendingBtn->setProperty("workspaceName", workspaceName);
            pendingBtn->setProperty("categoryName", it.key());
            pendingBtn->setProperty("taskDescription", task->getDescription());
            connect(pendingBtn, &QPushButton::clicked, this, [this, workspaceName, it, task]() {
                changeTaskStatus(workspaceName, it.key(), task->getDescription(),
                                 isEnglish ? "Pending" : "В ожидании");
            });

            QPushButton *progressBtn = new QPushButton(actions);
            progressBtn->setIcon(QIcon(":/icons/inprogress.png"));
            progressBtn->setToolTip(translate("В процессе"));
            progressBtn->setProperty("workspaceName", workspaceName);
            progressBtn->setProperty("categoryName", it.key());
            progressBtn->setProperty("taskDescription", task->getDescription());
            connect(progressBtn, &QPushButton::clicked, this, [this, workspaceName, it, task]() {
                changeTaskStatus(workspaceName, it.key(), task->getDescription(),
                                 isEnglish ? "In Progress" : "В процессе");
            });

            QPushButton *completeBtn = new QPushButton(actions);
            completeBtn->setIcon(QIcon(":/icons/completed.png"));
            completeBtn->setToolTip(translate("Завершено"));
            completeBtn->setProperty("workspaceName", workspaceName);
            completeBtn->setProperty("categoryName", it.key());
            completeBtn->setProperty("taskDescription", task->getDescription());
            connect(completeBtn, &QPushButton::clicked, this, [this, workspaceName, it, task]() {
                changeTaskStatus(workspaceName, it.key(), task->getDescription(),
                                 isEnglish ? "Completed" : "Завершено");
            });

            QPushButton *deleteBtn = new QPushButton(actions);
            deleteBtn->setIcon(QIcon(":/icons/delete.png"));
            deleteBtn->setToolTip(translate("Удалить"));
            deleteBtn->setProperty("workspaceName", workspaceName);
            deleteBtn->setProperty("categoryName", it.key());
            deleteBtn->setProperty("taskDescription", task->getDescription());
            connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::removeTask);

            actionsLayout->addWidget(pendingBtn);
            actionsLayout->addWidget(progressBtn);
            actionsLayout->addWidget(completeBtn);
            actionsLayout->addWidget(deleteBtn);
            actionsLayout->setContentsMargins(0, 0, 0, 0);
            table->setCellWidget(row, 5, actions);
            row++;
        }

        layout->addWidget(addTaskBtn);
        layout->addWidget(deleteBtn);
        layout->addWidget(table);
        categoriesLayout->addWidget(group);
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
    QString workspaceName = QInputDialog::getText(this, translate("Добавить рабочее пространство"),
                                                  translate("Имя рабочего пространства:"), QLineEdit::Normal, "", &ok);
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
    reply = QMessageBox::question(this, translate("Удалить рабочее пространство"),
                                  translate("Вы уверены, что хотите удалить рабочее пространство \"%1\" и ВСЕ его содержимое?").arg(workspaceName),
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
        QMessageBox::critical(this, translate("Ошибка"), translate("Не удалось удалить рабочее пространство"));
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

    workspaceName.replace(translate("Workspace: "), "");
    workspaceName.replace(translate("Рабочее пространство: "), "");

    showCategories(workspaceName);
}

void MainWindow::addCategory()
{
    QString currentText = currentWorkspaceLabel->text();
    QString workspaceName = currentText.replace(translate("Рабочее пространство: "), "").replace(tr("Workspace: "), "");

    if (!workspaces.contains(workspaceName)) return;

    bool ok;
    QString categoryName = QInputDialog::getText(this, translate("Добавить категорию"),
                                                 translate("Имя категории:"), QLineEdit::Normal, "", &ok);
    if (ok && !categoryName.isEmpty()) {
        QString sql = "INSERT INTO Categories (name, workspace_id) VALUES ('" + categoryName + "', " +
                      QString::number(workspaces[workspaceName]->getId()) + ");";
        executeSQL(sql);

        workspaces[workspaceName]->addCategory(categoryName);
        showCategories(workspaceName);
    }
}

QDialog* MainWindow::createNotificationDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(translate("Notifications"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    connect(this, &MainWindow::languageChanged, dialog, [=]() {
        dialog->setWindowTitle(translate("Notifications"));
    });

    return dialog;
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
    reply = QMessageBox::question(this, translate("Удалить категорию"),
                                  translate("Вы уверены, что хотите удалить категорию \"%1\"?").arg(categoryName),
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
    dialog.setWindowTitle(translate("Создать задачу"));

    QFormLayout form(&dialog);

    QLineEdit *descriptionEdit = new QLineEdit(&dialog);
    QLineEdit *tagsEdit = new QLineEdit(&dialog);

    QComboBox *difficultyCombo = new QComboBox(&dialog);
    difficultyCombo->addItems(QStringList() << translate("Лёгкая") << translate("Средняя") << translate("Сложная"));

    QComboBox *priorityCombo = new QComboBox(&dialog);
    priorityCombo->addItems(QStringList() << translate("Низкий") << translate("Средний") << translate("Высокий"));

    QDateEdit *deadlineEdit = new QDateEdit(&dialog);
    deadlineEdit->setDisplayFormat("dd-MM-yyyy");
    deadlineEdit->setDate(QDate::currentDate());
    deadlineEdit->setCalendarPopup(true);

    form.addRow(translate("Название задачи:"), descriptionEdit);
    form.addRow(translate("Тэги:"), tagsEdit);
    form.addRow(translate("Сложность:"), difficultyCombo);
    form.addRow(translate("Приоритет:"), priorityCombo);
    form.addRow(translate("Дата выполнения:"), deadlineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    buttonBox.button(QDialogButtonBox::Ok)->setText(translate("Создать"));
    buttonBox.button(QDialogButtonBox::Cancel)->setText(translate("Отмена"));
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
            QMessageBox::warning(this, translate("Ошибка"), translate("Название задачи не может быть пустым"));
            return;
        }

        QStringList tagList = tags.split(',', Qt::SkipEmptyParts);
        for (QString &tag : tagList) {
            tag = tag.trimmed();
        }

        Category *category = workspaces[workspaceName]->getCategories()[categoryName];

        QString sql = "INSERT INTO Tasks (description, category_id, difficulty, priority, status, deadline) VALUES ('" +
                      description + "', " + QString::number(category->getId()) + ", '" +
                      (isEnglish ?
                           (difficulty == "Лёгкая" ? "Easy" : difficulty == "Средняя" ? "Medium" : "Hard") :
                           difficulty) + "', '" +
                      (isEnglish ?
                           (priority == "Низкий" ? "Low" : priority == "Средний" ? "Medium" : "High") :
                           priority) + "', 'Pending', '" + deadline + "');";
        executeSQL(sql);

        int taskId = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();

        for (const QString &tag : tagList) {
            sql = "INSERT INTO TaskTags (task_id, tag) VALUES (" + QString::number(taskId) + ", '" + tag + "');";
            executeSQL(sql);
        }

        Task *task = new Task(taskId, description, categoryName, tagList,
                              isEnglish ?
                                  (difficulty == "Лёгкая" ? "Easy" : difficulty == "Средняя" ? "Medium" : "Hard") :
                                  difficulty,
                              isEnglish ?
                                  (priority == "Низкий" ? "Low" : priority == "Средний" ? "Medium" : "High") :
                                  priority,
                              "Pending", deadline);
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
    reply = QMessageBox::question(this, translate("Delete Task"),
                                  translate("Are you sure you want to delete task \"%1\"?").arg(taskDescription),
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
        QMessageBox::warning(this, translate("Error"), translate("Workspace not found"));
        return;
    }

    Workspace *workspace = workspaces[workspaceName];
    if (!workspace->getCategories().contains(categoryName)) {
        QMessageBox::warning(this, translate("Error"), translate("Category not found"));
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
        QMessageBox::warning(this, translate("Error"), translate("Task not found"));
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

        QMessageBox::information(this, translate("Task Completed"),
                                 translate("Task \"%1\" has been moved to history").arg(taskDescription));
    } else {
        QMessageBox::information(this, tr("Status Changed"),
                                 translate("Status for task \"%1\" has been updated").arg(taskDescription));
    }

    showCategories(workspaceName);
}

void MainWindow::showHistory()
{
    QDialog historyDialog(this);
    historyDialog.setWindowTitle(translate("История"));
    historyDialog.resize(800, 600);

    QVBoxLayout layout(&historyDialog);

    QTableWidget *historyTable = new QTableWidget(0, 6, &historyDialog);
    historyTable->setHorizontalHeaderLabels(QStringList() << translate("Задача") << translate("Категория") << translate("Срок")
                                                          << translate("Статус") << translate("Приоритет") << translate("Сложность"));
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

    QPushButton *restoreButton = new QPushButton(translate("Восстановить задачу"), &historyDialog);
    QPushButton *deleteButton = new QPushButton(translate("Удалить задачу"), &historyDialog);
    QPushButton *closeButton = new QPushButton(translate("Закрыть"), &historyDialog);

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
    QString taskDescription = QInputDialog::getText(this, translate("Restore Task"),
                                                    translate("Enter task description to restore:"),
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || taskDescription.isEmpty()) return;

    QString workspaceName = QInputDialog::getText(this, translate("Restore Task"),
                                                  translate("Enter workspace name:"),
                                                  QLineEdit::Normal, "", &ok);
    if (!ok || workspaceName.isEmpty()) return;

    QString categoryName = QInputDialog::getText(this, translate("Restore Task"),
                                                 translate("Enter category name:"),
                                                 QLineEdit::Normal, "", &ok);
    if (!ok || categoryName.isEmpty()) return;

    for (auto it = taskHistory.begin(); it != taskHistory.end(); ++it) {
        if (compareStringsIgnoreCase(it->getDescription(), taskDescription)) {
            if (!workspaces.contains(workspaceName)) {
                QMessageBox::warning(this, translate("Error"), translate("Workspace not found"));
                return;
            }

            Workspace *workspace = workspaces[workspaceName];
            if (!workspace->getCategories().contains(categoryName)) {
                QMessageBox::warning(this, translate("Error"), translate("Category not found in workspace"));
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

            QMessageBox::information(this, translate("Task Restored"),
                                     translate("Task \"%1\" has been restored").arg(taskDescription));
            showCategories(workspaceName);
            return;
        }
    }

    QMessageBox::warning(this, translate("Error"), translate("Task not found in history"));
}

void MainWindow::deleteTaskFromHistory()
{
    bool ok;
    QString taskDescription = QInputDialog::getText(this, translate("Delete Task"),
                                                    translate("Enter task description to delete:"),
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || taskDescription.isEmpty()) return;

    for (auto it = taskHistory.begin(); it != taskHistory.end(); ++it) {
        if (compareStringsIgnoreCase(it->getDescription(), taskDescription)) {
            QString sql = "DELETE FROM TaskHistory WHERE id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            taskHistory.erase(it);

            QMessageBox::information(this, translate("Task Deleted"),
                                     translate("Task \"%1\" has been permanently deleted").arg(taskDescription));
            return;
        }
    }

    QMessageBox::warning(this, translate("Error"), translate("Task not found in history"));
}

void MainWindow::showNotifications() {
    checkDeadlines();

    QDialog notificationsDialog(this);
    notificationsDialog.setWindowTitle(translate("Уведомления"));

    QVBoxLayout layout(&notificationsDialog);

    QVector<Notification> unviewedNotifications;
    for (const Notification &n : notifications) {
        if (!n.isViewed()) {
            unviewedNotifications.append(n);
        }
    }

    if (unviewedNotifications.isEmpty()) {
        QLabel *noNotificationsLabel = new QLabel(translate("Нет новых уведомлений"), &notificationsDialog);
        layout.addWidget(noNotificationsLabel);
    } else {
        QListWidget *notificationsList = new QListWidget(&notificationsDialog);
        for (const Notification &notification : unviewedNotifications) {
            notificationsList->addItem(notification.getMessage());
        }
        layout.addWidget(notificationsList);
    }

    QPushButton *clearButton = new QPushButton(translate("Очистить уведомления"), &notificationsDialog);
    QPushButton *closeButton = new QPushButton(translate("Закрыть"), &notificationsDialog);

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

void MainWindow::toggleLanguage() {
    isEnglish = !isEnglish;

    for (Notification &n : notifications) {
        n.updateMessage(isEnglish);
    }

    retranslateUi();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        updateUI();
    }
    QMainWindow::changeEvent(event);
}

QInputDialog* MainWindow::createInputDialog(const QString &title, const QString &label)
{
    QInputDialog* dialog = new QInputDialog(this);
    dialog->setWindowTitle(title);
    dialog->setLabelText(label);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(this, &MainWindow::languageChanged, dialog, [=]() {
        dialog->setWindowTitle(tr(qPrintable(title)));
        dialog->setLabelText(tr(qPrintable(label)));
    });

    return dialog;
}


void MainWindow::updateUI() {
    setWindowTitle(translate("Менеджер задач"));
    addWorkspaceButton->setText(translate("Добавить рабочее пространство"));
    addCategoryButton->setText(translate("Добавить категорию"));
    historyButton->setText(translate("История"));
    notificationsButton->setText(translate("Уведомления"));
    languageButton->setText(isEnglish ? translate("Русский") : translate("English"));

    QString currentText = currentWorkspaceLabel->text();
    if (currentText != translate("Выберите рабочее пространство")) {
        QString cleanName = currentText;
        cleanName.replace(translate("Рабочее пространство: "), "").replace(translate("Workspace: "), "");
        currentWorkspaceLabel->setText(translate("Рабочее пространство: %1").arg(cleanName));
    }

    showWorkspaces();
    if (!currentWorkspaceLabel->text().isEmpty() &&
        currentWorkspaceLabel->text() != translate("Выберите рабочее пространство")) {
        QString cleanName = currentWorkspaceLabel->text();
        cleanName.replace(translate("Рабочее пространство: "), "").replace(translate("Workspace: "), "");
        showCategories(cleanName);
    }

    QWidgetList widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets) {
        if (widget->isWindow() && widget != this) {
            if (QDialog *dialog = qobject_cast<QDialog*>(widget)) {
                QString title = dialog->windowTitle();
                if (title == "Создать задачу" || title == "Create Task") {
                    dialog->setWindowTitle(translate("Создать задачу"));
                } else if (title == "Уведомления" || title == "Notifications") {
                    dialog->setWindowTitle(translate("Уведомления"));
                } else if (title == "История" || title == "History") {
                    dialog->setWindowTitle(translate("История"));
                }
            }
        }
    }
}

void MainWindow::retranslateUi() {

    setWindowTitle(translate("Менеджер задач"));
    addWorkspaceButton->setText(translate("Добавить рабочее пространство"));
    addCategoryButton->setText(translate("Добавить категорию"));
    historyButton->setText(translate("История"));
    notificationsButton->setText(translate("Уведомления"));
    languageButton->setText(isEnglish ? "Русский" : "English");

    QString currentText = currentWorkspaceLabel->text();
    if (currentText != translate("Выберите рабочее пространство")) {
        QString cleanName = currentText;
        cleanName.replace(translate("Рабочее пространство: "), "")
            .replace(translate("Workspace: "), "");
        currentWorkspaceLabel->setText(translate("Рабочее пространство: %1").arg(cleanName));
    }

    if (!currentWorkspaceLabel->text().isEmpty()) {
        QString cleanName = currentWorkspaceLabel->text();
        cleanName.replace(translate("Рабочее пространство: "), "")
            .replace(translate("Workspace: "), "");
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
