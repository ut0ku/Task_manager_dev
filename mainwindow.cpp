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

// Вспомогательная ф-ция превода
static QString translate(const char* text) {
    return QCoreApplication::translate("MainWindow", text);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isEnglish(false), isDarkTheme(false)
{
    // Инициализация бд
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

    // Кнопки для темы
    themeButton = new QPushButton(translate("Темная тема"), this);
    connect(themeButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);

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

// Весь перевод
QString MainWindow::translate(const QString& text) const {
    static const QMap<QString, QString> translations = {

        // Основное окно
        {"Менеджер задач", "Task Manager"},
        {"Добавить рабочее пространство", "Add Workspace"},
        {"Добавить категорию", "Add Category"},
        {"История", "History"},
        {"Уведомления", "Notifications"},
        {"Выберите рабочее пространство", "Select a workspace"},
        {"Select a workspace", "Выберите рабочее пространство"},
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

        {"Easy", "Лёгкая"},
        {"Medium", "Средняя"},
        {"Hard", "Сложная"},
        {"Low", "Низкий"},
        {"Medium", "Средний"},
        {"High", "Высокий"},
        {"Лёгкая", "Easy"},
        {"Средняя", "Medium"},
        {"Сложная", "Hard"},
        {"Низкий", "Low"},
        {"Средний", "Medium"},
        {"Высокий", "High"},

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
        {"Задача удалена", "Task deleted"},
        {"Введите описание задачи для удаления:", "Enter task description to delete:"},
        {"Рабочее пространство не найдено", "Workspace not found"},
        {"Категория не найдена в рабочем пространстве","Category not found in workspace"},
        {"Была удалена навсегда", "has been permanently deleted"},
        {"Задача \"%1\" была восстановлена","Task \"%1\" has been restored"},

        // Поиск задача по тегам
        {"Поиск по тегам", "Search by Tags"},
        {"Поиск задач по тегам", "Search Tasks by Tags"},
        {"Введите теги (через запятую):", "Enter tags (comma separated):"},
        {"Результаты поиска", "Search Results"},
        {"Задачи с указанными тегами не найдены", "No tasks found with these tags"},
        {"Задачи найдены в:", "Tasks found in:"},
        {"Рабочее пространство", "Workspace"},
        {"Категория", "Category"},
        {"Закрыть", "Close"},

        // Кнопки смены темы
        {"Темная тема", "Dark Theme"},
        {"Светлая тема", "Light Theme"}

    };

    return isEnglish ? translations.value(text, text) : text;
}

// Настройка интерфейса
void MainWindow::setupUI()
{
    // Компоновка + основной виджет
    mainWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(mainWidget);

    // Sidebar
    sidebar = new QScrollArea(this);
    sidebar->setFixedWidth(250);
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
    toggleSidebarButton->raise(); // Кнопка поверх других виджетов

    // Рабочие пространства
    showWorkspaces();

    sidebar->setWidget(sidebarContent);

    // Область рабочей области
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

    // Правый sidebar
    QWidget *rightSidebar = new QWidget(this);
    QVBoxLayout *rightSidebarLayout = new QVBoxLayout(rightSidebar);
    rightSidebarLayout->setAlignment(Qt::AlignTop);

    historyButton = new QPushButton(translate("History"), this);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistory);

    notificationsButton = new QPushButton(translate("Notifications"), this);
    connect(notificationsButton, &QPushButton::clicked, this, &MainWindow::showNotifications);

    languageButton = new QPushButton(translate("English"), this);
    connect(languageButton, &QPushButton::clicked, this, &MainWindow::toggleLanguage);

    searchByTagsButton = new QPushButton(translate("Поиск по тегам"), this);
    connect(searchByTagsButton, &QPushButton::clicked, this, &MainWindow::searchTasksByTags);

    rightSidebarLayout->addWidget(themeButton);
    applyTheme(false);

    rightSidebarLayout->addWidget(historyButton);
    rightSidebarLayout->addWidget(notificationsButton);
    rightSidebarLayout->addWidget(languageButton);
    rightSidebarLayout->addWidget(searchByTagsButton);


    // Основной слой
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(sidebar);
    contentLayout->addWidget(workspaceView);
    contentLayout->addWidget(rightSidebar);

    mainLayout->addLayout(contentLayout);
    setCentralWidget(mainWidget);

    resize(1100, 650);
    setWindowTitle(translate("Task Manager"));

}

// Загрузка Workspaces из бд
void MainWindow::loadWorkspaces()
{
    QSqlQuery query("SELECT id, name FROM Workspaces;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();

        qDebug() << "Loading workspace - ID:" << id << "Name:" << name;

        if (id == 0) {
            qDebug() << "Warning: Workspace with ID 0 found! This should not happen.";
            continue;
        }

        workspaces[name] = new Workspace(id, name);
    }
}

// Загрузка Categories из бд
void MainWindow::loadCategories()
{
    QSqlQuery query("SELECT id, name, workspace_id FROM Categories;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        int workspaceId = query.value(2).toInt();

        qDebug() << "Loading category - ID:" << id << "Name:" << name
                 << "Workspace ID:" << workspaceId;

        bool categoryAdded = false;
        for (auto it = workspaces.begin(); it != workspaces.end(); ++it) {
            if (it.value()->getId() == workspaceId) {
                it.value()->addCategory(id, name);
                categoryAdded = true;
                qDebug() << "Added category to workspace:" << it.key()
                         << "with ID:" << id;
                break;
            }
        }

        if (!categoryAdded) {
            qDebug() << "Category" << name << "with ID" << id
                     << "has no matching workspace (Workspace ID:" << workspaceId << ")";
        }
    }
}

// Загрузка тасков из бд
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

        qDebug() << "Loading task ID:" << id << "Description:" << description
                 << "Category ID:" << categoryId;

        // Теги
        QStringList tags;
        QSqlQuery tagQuery;
        tagQuery.prepare("SELECT tag FROM TaskTags WHERE task_id = :task_id");
        tagQuery.bindValue(":task_id", id);

        if (tagQuery.exec()) {
            while (tagQuery.next()) {
                tags.append(tagQuery.value(0).toString());
            }
            qDebug() << "Tags for task" << id << ":" << tags;
        } else {
            qDebug() << "Error loading tags for task" << id << ":" << tagQuery.lastError().text();
        }

        // Поиск категории
        bool taskLoaded = false;
        for (auto workspaceIt = workspaces.begin(); workspaceIt != workspaces.end() && !taskLoaded; ++workspaceIt) {
            Workspace* workspace = workspaceIt.value();
            QMap<QString, Category*>& categories = workspace->getCategories();

            for (auto categoryIt = categories.begin(); categoryIt != categories.end() && !taskLoaded; ++categoryIt) {
                Category* category = categoryIt.value();

                if (category->getId() == categoryId) {
                    Task* task = new Task(id, description, category->getName(), tags,
                                          difficulty, priority, status, deadline);
                    category->addTask(task);
                    taskLoaded = true;

                    qDebug() << "Successfully loaded task into workspace:" << workspace->getName()
                             << "category:" << category->getName()
                             << "task ID:" << id;
                }
            }
        }

        if (!taskLoaded) {
            qDebug() << "Failed to load task - category ID" << categoryId << "not found for task ID:" << id;
        }
    }
}

// Загрузка истории из бд
void MainWindow::loadTaskHistory() {
    QSqlQuery query("SELECT id, description, category_id, difficulty, priority, status, deadline FROM TaskHistory;");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString description = query.value(1).toString();
        int categoryId = query.value(2).toInt();
        QString difficulty = query.value(3).toString();
        QString priority = query.value(4).toString();
        QString status = query.value(5).toString();
        QString deadline = query.value(6).toString();

        // Приведение статуса к текущему языку
        if (isEnglish && status == "Завершено") {
            status = "Completed";
        } else if (!isEnglish && status == "Completed") {
            status = "Завершено";
        }

        Task task(id, description, "", QStringList(), difficulty, priority, status, deadline);
        taskHistory.append(task);
    }
}

// Выполнение sql запроса
void MainWindow::executeSQL(const QString& sql)
{
    QSqlQuery query;
    if (!query.exec(sql)) {
        qDebug() << "SQL error:" << query.lastError().text();
    }
}

// Отображение Workspaces
void MainWindow::showWorkspaces()
{
    // Очищение предыдущих элементов (кроме первых 2х - меню & добавления)
    while (sidebarLayout->count() > 2) {
        QLayoutItem* item = sidebarLayout->takeAt(2);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Добавление рабочего пр-ва
    for (auto workspaceIt = workspaces.begin(); workspaceIt != workspaces.end(); ++workspaceIt) {
        QWidget* workspaceWidget = new QWidget(sidebarContent);
        QHBoxLayout* workspaceLayout = new QHBoxLayout(workspaceWidget);
        workspaceLayout->setContentsMargins(0, 0, 0, 0);
        workspaceLayout->setSpacing(5);

        // Кнопка выбора рабочего пр-ва
        QPushButton* workspaceBtn = new QPushButton(workspaceIt.key(), workspaceWidget);
        workspaceBtn->setProperty("workspaceName", workspaceIt.key());
        workspaceBtn->setMinimumWidth(160);
        workspaceBtn->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        connect(workspaceBtn, &QPushButton::clicked, this, &MainWindow::workspaceSelected);

        // Кнопка удаления
        QPushButton* deleteBtn = new QPushButton("×", workspaceWidget);
        deleteBtn->setProperty("workspaceName", workspaceIt.key());
        deleteBtn->setFixedSize(25, 25);
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::removeWorkspace);

        workspaceLayout->addWidget(workspaceBtn);
        workspaceLayout->addWidget(deleteBtn);
        sidebarLayout->addWidget(workspaceWidget);
    }

    // Добавление растягивающегося элемента внизу
    sidebarLayout->addStretch();
}

// Отображение категорий
void MainWindow::showCategories(const QString& workspaceName) {
    currentWorkspaceLabel->setText(translate("Рабочее пространство: %1").arg(workspaceName));
    addCategoryButton->setEnabled(true);

    // Очищение предыдущих категорий
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

        // Мин высота
        group->setMinimumHeight(350);

        QPushButton *addTaskBtn = new QPushButton(translate("Создать задачу"), group);
        addTaskBtn->setProperty("workspaceName", workspaceName);
        addTaskBtn->setProperty("categoryName", it.key());
        connect(addTaskBtn, &QPushButton::clicked, this, &MainWindow::addTask);

        QPushButton *deleteBtn = new QPushButton(translate("Удалить категорию"), group);
        deleteBtn->setProperty("workspaceName", workspaceName);
        deleteBtn->setProperty("categoryName", it.key());
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::removeCategory);

        QTableWidget *table = new QTableWidget(0, 6, group);
        table->installEventFilter(this);
        table->viewport()->installEventFilter(this);
        table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        table->verticalScrollBar()->setSingleStep(15);
        table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        table->horizontalScrollBar()->setSingleStep(20);
        table->setHorizontalHeaderLabels({
            translate("Задача"), translate("Срок"), translate("Статус"),
            translate("Приоритет"), translate("Сложность"), translate("Действия")
        });

        // Начальная ширина столбцов (px)
        table->setColumnWidth(0, 150);  // название
        table->setColumnWidth(1, 90);   // срок
        table->setColumnWidth(2, 90);   // статус
        table->setColumnWidth(3, 90);   // приоритет
        table->setColumnWidth(4, 90);   // сложность
        table->setColumnWidth(5, 120);  // кнопки действий

        table->horizontalHeader()->setStretchLastSection(false);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // только 1й столбец растягивается
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);

        int row = 0;
        for (Task *task : it.value()->getTasks()) {
            table->insertRow(row);

            // Ячейка с описанием задачи
            QTableWidgetItem *descriptionItem = new QTableWidgetItem(task->getDescription());
            descriptionItem->setToolTip(task->getDescription()); // Полный текст в подсказке
            descriptionItem->setFlags(descriptionItem->flags() ^ Qt::ItemIsEditable);
            table->setItem(row, 0, descriptionItem);

            // Остальные ячейки
            table->setItem(row, 1, new QTableWidgetItem(task->getDeadline()));

            // Перевод статуса
            QString status = task->getStatus();
            if (isEnglish) {
                if (status == "В ожидании") status = "Pending";
                else if (status == "В процессе") status = "In Progress";
                else if (status == "Завершено") status = "Completed";
            } else {
                if (status == "Pending") status = "В ожидании";
                else if (status == "In Progress") status = "В процессе";
                else if (status == "Completed") status = "Завершено";
            }
            table->setItem(row, 2, new QTableWidgetItem(status));

            // Перевод приоритета и сложности
            QString priority = translate(task->getPriority());
            QString difficulty = translate(task->getDifficulty());

            table->setItem(row, 3, new QTableWidgetItem(priority));
            table->setItem(row, 4, new QTableWidgetItem(difficulty));

            // Ячейка с действиями
            QWidget *actions = new QWidget(table);
            QHBoxLayout *actionsLayout = new QHBoxLayout(actions);
            actionsLayout->setContentsMargins(0, 0, 0, 0);
            actionsLayout->setSpacing(5);

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
            table->setCellWidget(row, 5, actions);

            // Переход к след строке
            row++;
        }

        layout->addWidget(addTaskBtn);
        layout->addWidget(deleteBtn);
        layout->addWidget(table);
        layout->addStretch();

        categoriesLayout->addWidget(group);
    }

    categoriesLayout->addStretch();
}

// Переключение видимости бок понели
void MainWindow::toggleSidebar()
{
    if (sidebar->width() > 50) {
        // Сворачивание
        sidebar->setFixedWidth(0);
        toggleSidebarButton->setText(">");
        toggleSidebarButton->setParent(this); // перенос кнопки в гл меню

        // Кнопка в левом верхнем углу
        toggleSidebarButton->move(20, 20);

        // Кнопка поверх элементов
        toggleSidebarButton->raise();
        toggleSidebarButton->show();
    } else {

        // Разврачивание
        sidebar->setFixedWidth(250);
        toggleSidebarButton->setText("<");
        toggleSidebarButton->setParent(sidebarContent); // возвращение в sidebar
        sidebarLayout->insertWidget(0, toggleSidebarButton);
    }
}

// Добавление рабочего пространства
void MainWindow::addWorkspace()
{
    // Запрос имени
    bool ok;
    QString workspaceName = QInputDialog::getText(this, translate("Добавить рабочее пространство"),
                                                  translate("Имя рабочего пространства:"), QLineEdit::Normal, "", &ok);
    if (ok && !workspaceName.isEmpty()) {
        db.transaction();
        try {
            // Вставка в бд
            QSqlQuery query;
            query.prepare("INSERT INTO Workspaces (name) VALUES (:name)");
            query.bindValue(":name", workspaceName);

            if (!query.exec()) {
                throw std::runtime_error(query.lastError().text().toStdString());
            }

            int workspaceId = query.lastInsertId().toInt();
            qDebug() << "Inserted workspace ID:" << workspaceId;

            workspaces[workspaceName] = new Workspace(workspaceId, workspaceName);
            db.commit();

            qDebug() << "Successfully added workspace:" << workspaceName
                     << "with ID:" << workspaceId;

            showWorkspaces();
        } catch (const std::exception& e) {
            db.rollback();
            QMessageBox::critical(this, translate("Ошибка"),
                                  translate("Не удалось создать рабочее пространство: ") + QString::fromStdString(e.what()));
            qDebug() << "Error adding workspace:" << e.what();
        }
    }
}

// Удаление workspace
void MainWindow::removeWorkspace()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    if (!workspaces.contains(workspaceName)) return;

    // Подтверждение
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, translate("Удалить рабочее пространство"),
                                  translate("Вы уверены, что хотите удалить рабочее пространство \"%1\" и ВСЕ его содержимое?").arg(workspaceName),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    Workspace *workspace = workspaces[workspaceName];

    // Удаление из бд
    int workspaceId = workspace->getId();

    db.transaction();
    try {
        // Удаление всех задач + тегов
        QSqlQuery deleteTasksQuery;
        deleteTasksQuery.prepare(
            "DELETE FROM Tasks WHERE id IN ("
            "SELECT t.id FROM Tasks t "
            "JOIN Categories c ON t.category_id = c.id "
            "WHERE c.workspace_id = :workspace_id"
            ")"
            );
        deleteTasksQuery.bindValue(":workspace_id", workspaceId);
        if (!deleteTasksQuery.exec()) {
            throw std::runtime_error(deleteTasksQuery.lastError().text().toStdString());
        }

        // Удаление всех категорий
        QSqlQuery deleteCategoriesQuery;
        deleteCategoriesQuery.prepare("DELETE FROM Categories WHERE workspace_id = :workspace_id");
        deleteCategoriesQuery.bindValue(":workspace_id", workspaceId);
        if (!deleteCategoriesQuery.exec()) {
            throw std::runtime_error(deleteCategoriesQuery.lastError().text().toStdString());
        }

        // Удаление workspace
        QSqlQuery deleteWorkspaceQuery;
        deleteWorkspaceQuery.prepare("DELETE FROM Workspaces WHERE id = :workspace_id");
        deleteWorkspaceQuery.bindValue(":workspace_id", workspaceId);
        if (!deleteWorkspaceQuery.exec()) {
            throw std::runtime_error(deleteWorkspaceQuery.lastError().text().toStdString());
        }

        db.commit();

        delete workspace;
        workspaces.remove(workspaceName);
        showWorkspaces();

        updateUI();

        qDebug() << "Workspace deleted successfully. ID:" << workspaceId;
    } catch (const std::exception &e) {
        db.rollback();
        QMessageBox::critical(this, translate("Ошибка"),
                              translate("Не удалось удалить рабочее пространство: ") + QString::fromStdString(e.what()));
        qDebug() << "Error deleting workspace:" << e.what();
    }
}

// Выбор workspace'а
void MainWindow::workspaceSelected() {
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->text();

    // Удаление возможных префиксов перевода
    workspaceName.replace(translate("Workspace: "), "");
    workspaceName.replace(translate("Рабочее пространство: "), "");

    currentWorkspaceLabel->setText(translate("Рабочее пространство: %1").arg(workspaceName));
    showCategories(workspaceName);
}

// Добавление категории
void MainWindow::addCategory()
{
    qDebug() << "Starting addCategory method";

    QString currentText = currentWorkspaceLabel->text();
    QString workspaceName = currentText.replace(translate("Рабочее пространство: "), "").replace(tr("Workspace: "), "");

    if (!workspaces.contains(workspaceName)) {
        qDebug() << "Workspace not found:" << workspaceName;
        return;
    }

    // Запрос имени новой категории
    bool ok;
    QString categoryName = QInputDialog::getText(this, translate("Добавить категорию"),
                                                 translate("Имя категории:"), QLineEdit::Normal, "", &ok);
    if (ok && !categoryName.isEmpty()) {
        // Вставка в бд
        db.transaction();

        try {
            QSqlQuery query;
            query.prepare("INSERT INTO Categories (name, workspace_id) VALUES (:name, :workspace_id)");
            query.bindValue(":name", categoryName);
            query.bindValue(":workspace_id", workspaces[workspaceName]->getId());

            if (!query.exec()) {
                throw std::runtime_error(query.lastError().text().toStdString());
            }

            int categoryId = query.lastInsertId().toInt();
            qDebug() << "Inserted category ID:" << categoryId;

            workspaces[workspaceName]->addCategory(categoryId, categoryName);

            db.commit();
            showCategories(workspaceName);

            qDebug() << "Successfully added category:" << categoryName
                     << "with ID:" << categoryId
                     << "to workspace:" << workspaceName;
        } catch (const std::exception& e) {
            db.rollback();
            QMessageBox::critical(this, translate("Ошибка"),
                                  translate("Не удалось создать категорию: ") + QString::fromStdString(e.what()));
            qDebug() << "Error adding category:" << e.what();
        }
    }
}

// Диалог уведов
QDialog* MainWindow::createNotificationDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(translate("Notifications"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // Обновление перевода
    connect(this, &MainWindow::languageChanged, dialog, [=]() {
        dialog->setWindowTitle(translate("Notifications"));
    });

    return dialog;
}

// Удаление категории
void MainWindow::removeCategory()
{
    // Получение имени workspace'а + категории из свойств кнопки
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    QString categoryName = button->property("categoryName").toString();

    // Проверка на сущ + подтверждение удаления
    if (!workspaces.contains(workspaceName)) return;

    Workspace *workspace = workspaces[workspaceName];
    if (!workspace->getCategories().contains(categoryName)) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, translate("Удалить категорию"),
                                  translate("Вы уверены, что хотите удалить категорию \"%1\"?").arg(categoryName),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    Category *category = workspace->getCategories()[categoryName];
    int categoryId = category->getId();

    db.transaction();
    try {
        // Удаление всех здач + тегов
        QSqlQuery deleteTasksQuery;
        deleteTasksQuery.prepare(
            "DELETE FROM Tasks WHERE id IN ("
            "SELECT t.id FROM Tasks t "
            "WHERE t.category_id = :category_id"
            ")"
            );
        deleteTasksQuery.bindValue(":category_id", categoryId);
        if (!deleteTasksQuery.exec()) {
            throw std::runtime_error(deleteTasksQuery.lastError().text().toStdString());
        }

        // Удаление категории
        QSqlQuery deleteCategoryQuery;
        deleteCategoryQuery.prepare("DELETE FROM Categories WHERE id = :category_id");
        deleteCategoryQuery.bindValue(":category_id", categoryId);
        if (!deleteCategoryQuery.exec()) {
            throw std::runtime_error(deleteCategoryQuery.lastError().text().toStdString());
        }

        db.commit();

        // Удаление из памяти
        workspace->removeCategory(categoryName);
        showCategories(workspaceName);

        qDebug() << "Category deleted successfully. ID:" << categoryId;
    } catch (const std::exception &e) {
        db.rollback();
        QMessageBox::critical(this, translate("Ошибка"),
                              translate("Не удалось удалить категорию: ") + QString::fromStdString(e.what()));
        qDebug() << "Error deleting category:" << e.what();
    }
}

// Добавление таски
void MainWindow::addTask()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QString workspaceName = button->property("workspaceName").toString();
    QString categoryName = button->property("categoryName").toString();

    // Проверка на сущ
    if (!workspaces.contains(workspaceName) ||
        !workspaces[workspaceName]->getCategories().contains(categoryName)) {
        qDebug() << "Workspace or category not found:" << workspaceName << categoryName;
        return;
    }

    // Диалог для ввода
    QDialog dialog(this);
    dialog.setWindowTitle(translate("Создать задачу"));
    dialog.resize(350, 250);

    QFormLayout form(&dialog);
    // Поля ввода
    QLineEdit *descriptionEdit = new QLineEdit(&dialog);
    QLineEdit *tagsEdit = new QLineEdit(&dialog);

    QComboBox *difficultyCombo = new QComboBox(&dialog);
    difficultyCombo->addItems(QStringList()
                              << translate("Лёгкая") << translate("Средняя") << translate("Сложная"));

    difficultyCombo->setMinimumWidth(120);  // Увеличение мин ширины

    QComboBox *priorityCombo = new QComboBox(&dialog);
    priorityCombo->addItems(QStringList()
                            << translate("Низкий") << translate("Средний") << translate("Высокий"));

    priorityCombo->setMinimumWidth(120);  // Увеличение мин ширины

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

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString description = descriptionEdit->text();
        QString tags = tagsEdit->text();
        QString difficulty = difficultyCombo->currentText();
        QString priority = priorityCombo->currentText();
        QString deadline = deadlineEdit->date().toString("dd-MM-yyyy");

        if (description.isEmpty()) {
            QMessageBox::warning(this, translate("Ошибка"),
                                 translate("Название задачи не может быть пустым"));
            return;
        }

        // Теги (обработка)
        QStringList tagList = tags.split(',', Qt::SkipEmptyParts);
        for (QString &tag : tagList) {
            tag = tag.trimmed();
        }

        Category *category = workspaces[workspaceName]->getCategories()[categoryName];

        db.transaction();
        try {
            // Установка статуса взависимости от текущего языка
            QString status = isEnglish ? "Pending" : "В ожидании";

            // Вставка задачи
            QSqlQuery taskQuery;
            taskQuery.prepare(
                "INSERT INTO Tasks (description, category_id, difficulty, priority, status, deadline) "
                "VALUES (:description, :category_id, :difficulty, :priority, :status, :deadline)"
                );
            taskQuery.bindValue(":description", description);
            taskQuery.bindValue(":category_id", category->getId());
            taskQuery.bindValue(":difficulty", difficulty);
            taskQuery.bindValue(":priority", priority);
            taskQuery.bindValue(":status", status);
            taskQuery.bindValue(":deadline", deadline);

            if (!taskQuery.exec()) {
                throw std::runtime_error(taskQuery.lastError().text().toStdString());
            }

            // Получение ID
            int taskId = taskQuery.lastInsertId().toInt();
            qDebug() << "Inserted task ID:" << taskId;

            // Вставка тегов
            for (const QString &tag : tagList) {
                QSqlQuery tagQuery;
                tagQuery.prepare("INSERT INTO TaskTags (task_id, tag) VALUES (:task_id, :tag)");
                tagQuery.bindValue(":task_id", taskId);
                tagQuery.bindValue(":tag", tag);

                if (!tagQuery.exec()) {
                    throw std::runtime_error(tagQuery.lastError().text().toStdString());
                }
            }

            db.commit();

            // Добавление в память
            Task *task = new Task(taskId, description, categoryName, tagList,
                                  difficulty, priority, status, deadline);
            category->addTask(task);

            qDebug() << "Added task to category:" << categoryName
                     << "in workspace:" << workspaceName
                     << "with ID:" << taskId;

            showCategories(workspaceName);
        } catch (const std::exception &e) {
            db.rollback();
            QMessageBox::critical(this, translate("Ошибка"),
                                  translate("Не удалось сохранить задачу: ") + QString::fromStdString(e.what()));
            qDebug() << "Error adding task:" << e.what();
        }
    }
}

// Удаление таски
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

    // Поиск для удаления
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

    // Подтверждение
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, translate("Delete Task"),
                                  translate("Вы уверены, что хотите удалить задачу \"%1\"?").arg(taskDescription),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    db.transaction();

    try {
        // Удаление тегов
        QSqlQuery deleteTagsQuery;
        deleteTagsQuery.prepare("DELETE FROM TaskTags WHERE task_id = :task_id");
        deleteTagsQuery.bindValue(":task_id", taskToDelete->getId());
        if (!deleteTagsQuery.exec()) {
            throw std::runtime_error(deleteTagsQuery.lastError().text().toStdString());
        }

        // Удаление таски
        QSqlQuery deleteTaskQuery;
        deleteTaskQuery.prepare("DELETE FROM Tasks WHERE id = :task_id");
        deleteTaskQuery.bindValue(":task_id", taskToDelete->getId());
        if (!deleteTaskQuery.exec()) {
            throw std::runtime_error(deleteTaskQuery.lastError().text().toStdString());
        }

        db.commit();

        // Удаление из памяти
        delete category->getTasks().takeAt(taskIndex);
        showCategories(workspaceName);

        // Дебаги
        qDebug() << "Task deleted successfully. ID:" << taskToDelete->getId();
    } catch (const std::exception &e) {
        db.rollback();
        QMessageBox::critical(this, translate("Ошибка"),
                              translate("Не удалось удалить задачу: ") + QString::fromStdString(e.what()));
        qDebug() << "Error deleting task:" << e.what();
    }
}

// Изменение статуса задачи
void MainWindow::changeTaskStatus(const QString& workspaceName, const QString& categoryName,
                                  const QString& taskDescription, const QString& newStatus)
{
    // Проверка на сущ
    if (!workspaces.contains(workspaceName)) {
        QMessageBox::warning(this, translate("Ошибка"), translate("Рабочее пространство не найдено"));
        return;
    }

    Workspace *workspace = workspaces[workspaceName];
    if (!workspace->getCategories().contains(categoryName)) {
        QMessageBox::warning(this, translate("Ошибка"), translate("Категория не найдена"));
        return;
    }

    // Поиск
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
        QMessageBox::warning(this, translate("Ошибка"), translate("Задача не найдена"));
        return;
    }

    QString statusToSet = newStatus;

    if (isEnglish) {
        if (statusToSet == "В ожидании") statusToSet = "Pending";
        else if (statusToSet == "В процессе") statusToSet = "In Progress";
        else if (statusToSet == "Завершено") statusToSet = "Completed";
    } else {
        if (statusToSet == "Pending") statusToSet = "В ожидании";
        else if (statusToSet == "In Progress") statusToSet = "В процессе";
        else if (statusToSet == "Completed") statusToSet = "Завершено";
    }


    QString currentStatus = taskToComplete->getStatus();

    // Проверка на завершение таски (с учетом перевода)
    bool isCompleting = (newStatus == "Завершено" || newStatus == "Completed") &&
                        (currentStatus != "Завершено" && currentStatus != "Completed");

    // Обновление в бд
    QString sql = QString("UPDATE Tasks SET status = '%1' WHERE id = %2")
                      .arg(statusToSet)
                      .arg(taskToComplete->getId());
    executeSQL(sql);

    taskToComplete->setStatus(statusToSet);

    if (isCompleting) {
        // Перенос задачи в историю (если выполнена)
        sql = QString("INSERT INTO TaskHistory (description, category_id, difficulty, priority, status, deadline) "
                      "VALUES ('%1', %2, '%3', '%4', '%5', '%6')")
                  .arg(taskToComplete->getDescription())
                  .arg(category->getId())
                  .arg(taskToComplete->getDifficulty())
                  .arg(taskToComplete->getPriority())
                  .arg(isEnglish ? "Completed" : "Завершено")
                  .arg(taskToComplete->getDeadline());
        executeSQL(sql);

        int historyId = QSqlQuery("SELECT last_insert_rowid();").value(0).toInt();

        // Копирование тегов
        for (const QString &tag : taskToComplete->getTags()) {
            executeSQL(QString("INSERT INTO TaskTags (task_id, tag) VALUES (%1, '%2')")
                           .arg(historyId).arg(tag));
        }

        // Удаление из активных задач
        executeSQL(QString("DELETE FROM Tasks WHERE id = %1").arg(taskToComplete->getId()));
        executeSQL(QString("DELETE FROM TaskTags WHERE task_id = %1").arg(taskToComplete->getId()));

        // Обновление данных
        Task historyTask(taskToComplete->getId(), taskToComplete->getDescription(),
                         categoryName, taskToComplete->getTags(),
                         taskToComplete->getDifficulty(), taskToComplete->getPriority(),
                         isEnglish ? "Completed" : "Завершено", taskToComplete->getDeadline());
        taskHistory.append(historyTask);

        // Удаление задачи из категории
        delete category->getTasks().takeAt(taskIndex);

        QMessageBox::information(this, translate("Задача завершена"),
                                 translate("Задача \"%1\" перемещена в историю").arg(taskDescription));
    } else {
        QMessageBox::information(this, translate("Статус изменен"),
                                 translate("Статус задачи \"%1\" обновлен").arg(taskDescription));
    }

    showCategories(workspaceName);
}

// тест
// Проврка на завершенность статуса
bool MainWindow::isCompletedStatus(const QString& status) const {
    return compareStringsIgnoreCase(status, "Завершено") ||
           compareStringsIgnoreCase(status, "Completed");
}

// Отображение истории
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

    // Заполнение данными
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

    // Кнопки управления
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

// Возвращение задачи из истории
void MainWindow::restoreTaskFromHistory()
{
    // Запрос данных
    bool ok;
    QString taskDescription = QInputDialog::getText(this, translate("Восстановить задачу"),
                                                    translate("Введите описание задачи для восстановления:"),
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || taskDescription.isEmpty()) return;

    QString workspaceName = QInputDialog::getText(this, translate("Восстановить задачу"),
                                                  translate("Введите имя рабочего пространства:"),
                                                  QLineEdit::Normal, "", &ok);
    if (!ok || workspaceName.isEmpty()) return;

    QString categoryName = QInputDialog::getText(this, translate("Восстановить задачу"),
                                                 translate("Введите имя категории:"),
                                                 QLineEdit::Normal, "", &ok);
    if (!ok || categoryName.isEmpty()) return;

    // Поиск задач
    for (auto it = taskHistory.begin(); it != taskHistory.end(); ++it) {
        if (compareStringsIgnoreCase(it->getDescription(), taskDescription)) {
            if (!workspaces.contains(workspaceName)) {
                QMessageBox::warning(this, translate("Error"), translate("Рабочее пространство не найдено"));
                return;
            }

            Workspace *workspace = workspaces[workspaceName];
            if (!workspace->getCategories().contains(categoryName)) {
                QMessageBox::warning(this, translate("Error"), translate("Категория не найдена в рабочем пространстве"));
                return;
            }

            Category *category = workspace->getCategories()[categoryName];

            // Вставка в активные задачи
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

            // Удаление из истории
            sql = "DELETE FROM TaskHistory WHERE id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            // Создание новой задачи и добавление в категорию
            Task *task = new Task(taskId, it->getDescription(), categoryName,
                                  it->getTags(), it->getDifficulty(),
                                  it->getPriority(), it->getStatus(),
                                  it->getDeadline());
            category->addTask(task);

            taskHistory.erase(it);

            QMessageBox::information(this, translate("Задача восстановлена"),
                                     translate("Задача \"%1\" была восстановлена").arg(taskDescription));
            showCategories(workspaceName);
            return;
        }
    }

    QMessageBox::warning(this, translate("Error"), translate("Задача не найдена в истории"));
}

// Удаление таски из истории
void MainWindow::deleteTaskFromHistory()
{
    // Запрос описания задачи для удаления
    bool ok;
    QString taskDescription = QInputDialog::getText(this, translate("Удалить задачу"),
                                                    translate("Введите описание задачи для удаления:"),
                                                    QLineEdit::Normal, "", &ok);
    if (!ok || taskDescription.isEmpty()) return;

    for (auto it = taskHistory.begin(); it != taskHistory.end(); ++it) {
        if (compareStringsIgnoreCase(it->getDescription(), taskDescription)) {
            QString sql = "DELETE FROM TaskHistory WHERE id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            sql = "DELETE FROM TaskTags WHERE task_id = " + QString::number(it->getId()) + ";";
            executeSQL(sql);

            taskHistory.erase(it);

            QMessageBox::information(this, translate("Задача удалена"),
                                     translate("Task \"%1\" Была удалена навсегда").arg(taskDescription));
            return;
        }
    }

    QMessageBox::warning(this, translate("Error"), translate("Задача не найдена в истории"));
}

// Отображение уведомлений
void MainWindow::showNotifications() {
    checkDeadlines();

    QDialog notificationsDialog(this);
    notificationsDialog.setWindowTitle(translate("Уведомления"));
    notificationsDialog.resize(500, 300); // Размер окна

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

        // Плавный скролл
        notificationsList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);  // Прокрутка (px)
        notificationsList->verticalScrollBar()->setSingleStep(5);  // Шаг скролла
        notificationsList->setStyleSheet(
            "QScrollBar:vertical {"
            "    border: none;"
            "    background: #f0f0f0;"
            "    width: 10px;"
            "    margin: 0px 0px 0px 0px;"
            "}"
            "QScrollBar::handle:vertical {"
            "    background: #c0c0c0;"
            "    min-height: 20px;"
            "    border-radius: 5px;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "    height: 0px;"
            "}"
            );

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

// Проверка дедлайнов
void MainWindow::checkDeadlines()
{
    QDate currentDate = QDate::currentDate();

    // Перебор
    for (auto workspaceIt = workspaces.begin(); workspaceIt != workspaces.end(); ++workspaceIt) {
        QMap<QString, Category*>& categories = workspaceIt.value()->getCategories();
        for (auto categoryIt = categories.begin(); categoryIt != categories.end(); ++categoryIt) {
            QVector<Task*>& tasks = categoryIt.value()->getTasks();
            for (Task *task : tasks) {
                QString taskDeadline = task->getDeadline();
                if (!taskDeadline.isEmpty()) {
                    QDate deadlineDate = QDate::fromString(taskDeadline, "dd-MM-yyyy");
                    if (deadlineDate.isValid() && deadlineDate == currentDate) {

                        // Пр-ка
                        bool exists = false;
                        for (const Notification &n : notifications) {
                            if (n.getTaskDescription() == task->getDescription() &&
                                n.getDeadline() == taskDeadline) {
                                exists = true;
                                break;
                            }
                        }
                        // Добавление нового уведомления
                        if (!exists) {
                            notifications.append(Notification(task->getDescription(), taskDeadline));
                        }
                    }
                }
            }
        }
    }
}

// Очистка уведомлений
void MainWindow::clearNotifications()
{
    // test1
    // пометка "просмотренные"
    for (Notification &n : notifications) {
        n.markAsViewed();
    }

    // test2
    // Полное удаление:
    // notifications.clear();
}

// Смена языка
void MainWindow::toggleLanguage() {
    isEnglish = !isEnglish;

    // Обновление уведомлений + полное обновление интерфейса
    for (Notification &n : notifications) {
        n.updateMessage(isEnglish);
    }

    retranslateUi();
}

// Обработчик событий изменения языка
void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        updateUI();
    }
    QMainWindow::changeEvent(event);
}

// Диалог ввода
QInputDialog* MainWindow::createInputDialog(const QString &title, const QString &label)
{
    QInputDialog* dialog = new QInputDialog(this);
    dialog->setWindowTitle(title);
    dialog->setLabelText(label);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Обновление перевода при изменении языка
    connect(this, &MainWindow::languageChanged, dialog, [=]() {
        dialog->setWindowTitle(tr(qPrintable(title)));
        dialog->setLabelText(tr(qPrintable(label)));
    });

    return dialog;
}

// Обновление интерфейса
void MainWindow::updateUI() {
    setWindowTitle(translate("Менеджер задач"));
    addWorkspaceButton->setText(translate("Добавить рабочее пространство"));
    addCategoryButton->setText(translate("Добавить категорию"));
    historyButton->setText(translate("История"));
    notificationsButton->setText(translate("Уведомления"));
    languageButton->setText(isEnglish ? translate("Русский") : translate("English"));
    searchByTagsButton->setText(translate("Поиск по тегам"));
    themeButton->setText(isDarkTheme ? translate("Светлая тема") : translate("Темная тема"));

    // Обновление отображения рабочего пространства
    QString currentText = currentWorkspaceLabel->text();
    QString selectWorkspaceText = isEnglish ? translate("Select a workspace") : translate("Выберите рабочее пространство");
    if (currentText == translate("Выберите рабочее пространство") ||
        currentText == translate("Select a workspace")) {
        currentWorkspaceLabel->setText(selectWorkspaceText);
    } else {
        // Если уже выбрано рабочее пространство
        QString cleanName = currentText;
        cleanName.replace(translate("Рабочее пространство: "), "")
            .replace(translate("Workspace: "), "");
        currentWorkspaceLabel->setText(translate("Рабочее пространство: %1").arg(cleanName));
    }

    showWorkspaces();
    if (!currentWorkspaceLabel->text().isEmpty() &&
        currentWorkspaceLabel->text() != translate("Выберите рабочее пространство") &&
        currentWorkspaceLabel->text() != translate("Select a workspace")) {
        QString cleanName = currentWorkspaceLabel->text();
        cleanName.replace(translate("Рабочее пространство: "), "")
            .replace(translate("Workspace: "), "");
        showCategories(cleanName);
    }

    retranslateUi();

    // Обновление открытых диалогов
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

// Полное обновление перевода
void MainWindow::retranslateUi() {
    // Основные элементы
    setWindowTitle(translate("Менеджер задач"));
    addWorkspaceButton->setText(translate("Добавить рабочее пространство"));
    addCategoryButton->setText(translate("Добавить категорию"));
    historyButton->setText(translate("История"));
    notificationsButton->setText(translate("Уведомления"));
    languageButton->setText(isEnglish ? "Русский" : "English");
    searchByTagsButton->setText(translate("Поиск по тегам"));

    QString currentText = currentWorkspaceLabel->text();
    QString cleanName = currentText;

    cleanName.remove(translate("Workspace: "))
        .remove(translate("Рабочее пространство: "));

    if (cleanName.isEmpty() ||
        cleanName == translate("Выберите рабочее пространство") ||
        cleanName == translate("Select a workspace")) {
        currentWorkspaceLabel->setText(translate("Выберите рабочее пространство"));
    } else {
        currentWorkspaceLabel->setText(
            translate(isEnglish ? "Workspace: %1" : "Рабочее пространство: %1")
                .arg(cleanName));
    }

    // Кнопки темы
    themeButton->setText(isDarkTheme ? translate("Светлая тема") : translate("Темная тема"));

    // Текущее рабочее пространство


    // Обновление отображения категорий и задач
    if (!currentWorkspaceLabel->text().isEmpty() &&
        currentWorkspaceLabel->text() != translate("Выберите рабочее пространство")) {
        QString cleanName = currentWorkspaceLabel->text();
        cleanName.replace(translate("Рабочее пространство: "), "")
            .replace(translate("Workspace: "), "");
        showCategories(cleanName);
    }
}

// Поиск по тегам
void MainWindow::searchTasksByTags() {
    bool ok;
    QString tagsInput = QInputDialog::getText(this, translate("Поиск задач по тегам"),
                                              translate("Введите теги (через запятую):"),
                                              QLineEdit::Normal, "", &ok);
    if (!ok || tagsInput.isEmpty()) return;

    QStringList tags = tagsInput.split(',', Qt::SkipEmptyParts);
    for (QString& tag : tags) {
        tag = tag.trimmed();
    }

    QVector<QPair<QString, QString>> results = findTasksByTags(tags);

    QDialog resultsDialog(this);
    resultsDialog.setWindowTitle(translate("Результаты поиска"));
    resultsDialog.resize(400, 300);

    QVBoxLayout layout(&resultsDialog);

    if (results.isEmpty()) {
        QLabel* noResultsLabel = new QLabel(translate("Задачи с указанными тегами не найдены"), &resultsDialog);
        layout.addWidget(noResultsLabel);
    } else {
        QLabel* resultsLabel = new QLabel(translate("Задачи найдены в:"), &resultsDialog);
        layout.addWidget(resultsLabel);

        QTableWidget* resultsTable = new QTableWidget(0, 2, &resultsDialog);
        resultsTable->setHorizontalHeaderLabels({translate("Рабочее пространство"), translate("Категория")});

        resultsTable->setColumnWidth(0, 180);  // +30!
        resultsTable->setColumnWidth(1, 150);

        resultsTable->horizontalHeader()->setStretchLastSection(true);
        resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // Удаление дубликатов
        QSet<QPair<QString, QString>> uniqueResults;
        for (const auto& result : results) {
            uniqueResults.insert(result);
        }

        resultsTable->setRowCount(uniqueResults.size());
        int row = 0;
        for (const auto& result : uniqueResults) {
            resultsTable->setItem(row, 0, new QTableWidgetItem(result.first));
            resultsTable->setItem(row, 1, new QTableWidgetItem(result.second));
            row++;
        }

        layout.addWidget(resultsTable);
    }

    QPushButton* closeButton = new QPushButton(translate("Закрыть"), &resultsDialog);
    layout.addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, &resultsDialog, &QDialog::accept);

    resultsDialog.exec();
}

// Возвращение строки в нижнем реигстре
QString MainWindow::toLowerCase(const QString& str) const
{
    return str.toLower();
}

// Сравнение строк (без учета регистра)
bool MainWindow::compareStringsIgnoreCase(const QString& a, const QString& b) const
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

// Настройка Scroll'а
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        QWidget *widget = qobject_cast<QWidget*>(obj);
        if (widget) {
            QTableWidget *table = nullptr;
            if (widget->inherits("QTableWidget")) {
                table = qobject_cast<QTableWidget*>(widget);
            } else if (widget->parent() && widget->parent()->inherits("QTableWidget")) {
                table = qobject_cast<QTableWidget*>(widget->parent());
            }

            if (table) {
                QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
                QScrollBar *hScroll = table->horizontalScrollBar();
                QScrollBar *vScroll = table->verticalScrollBar();

                // Проверка границ скролла (горизонтального)
                bool atLeft = (hScroll->value() == hScroll->minimum());
                bool atRight = (hScroll->value() == hScroll->maximum());

                // Проверка границ скролла (вертикального)
                bool atTop = (vScroll->value() == vScroll->minimum());
                bool atBottom = (vScroll->value() == vScroll->maximum());

                // Гор scroll
                if (abs(wheelEvent->angleDelta().x()) > abs(wheelEvent->angleDelta().y())) {
                    // Если слева и прокрутка влево - блокируем
                    if (atLeft && wheelEvent->angleDelta().x() > 0) {
                        return true;
                    }
                    // Если справа и прокрутка вправо - блокируем
                    if (atRight && wheelEvent->angleDelta().x() < 0) {
                        return true;
                    }

                    return false;
                }
                // Верт scroll
                else {
                    // Если вверху и прокрутка вверх - блокируем
                    if (atTop && wheelEvent->angleDelta().y() > 0) {
                        return true;
                    }
                    // Если внизу и прокрутка вниз - блокируем
                    if (atBottom && wheelEvent->angleDelta().y() < 0) {
                        return true;
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Нахождение задач по тегам
QVector<QPair<QString, QString>> MainWindow::findTasksByTags(const QStringList& tags) {
    QVector<QPair<QString, QString>> results;
    qDebug() << "Starting tag search for tags:" << tags;

    // Проверка активных тасков
    for (auto workspaceIt = workspaces.begin(); workspaceIt != workspaces.end(); ++workspaceIt) {
        Workspace* workspace = workspaceIt.value();
        QMap<QString, Category*>& categories = workspace->getCategories();

        for (auto categoryIt = categories.begin(); categoryIt != categories.end(); ++categoryIt) {
            Category* category = categoryIt.value();
            QVector<Task*>& tasks = category->getTasks();

            for (Task* task : tasks) {
                QStringList taskTags = task->getTags();
                qDebug() << "Checking task:" << task->getDescription() << "with tags:" << taskTags;

                for (const QString& taskTag : taskTags) {
                    for (const QString& searchTag : tags) {
                        // Сравнение (без регистра)
                        if (taskTag.trimmed().compare(searchTag.trimmed(), Qt::CaseInsensitive) == 0) {
                            results.append(qMakePair(workspace->getName(), category->getName()));
                            qDebug() << "Found match in active tasks! Workspace:" << workspace->getName()
                                     << "Category:" << category->getName() << "Task:" << task->getDescription();
                            goto next_task;
                        }
                    }
                }
            next_task:;
            }
        }
    }
    //Дебаг
    if (results.isEmpty()) {
        qDebug() << "No tasks found with these tags. All available tags in the system:";
        QSet<QString> allTags;

        // Сборка тегов
        for (const auto& workspace : workspaces) {
            for (const auto& category : workspace->getCategories()) {
                for (const Task* task : category->getTasks()) {
                    for (const QString& tag : task->getTags()) {
                        if (!tag.isEmpty()) {
                            allTags.insert(tag.toLower());
                        }
                    }
                }
            }
        }

        qDebug() << "All existing tags:" << allTags.values();
    }

    return results;
}

// Переключение темы
void MainWindow::toggleTheme()
{
    isDarkTheme = !isDarkTheme;
    applyTheme(isDarkTheme);
    themeButton->setText(isDarkTheme ? translate("Светлая тема") : translate("Темная тема"));
}

// Применение темы
void MainWindow::applyTheme(bool dark)
{
    if (dark) {
        currentThemeStyle = R"(
            /* Dark Theme */
            QWidget {
                background-color: #2d2d2d;
                color: #e0e0e0;
                border: none;
            }

            QMainWindow, QDialog {
                background-color: #2d2d2d;
            }

            QPushButton {
                background-color: #3a3a3a;
                color: #e0e0e0;
                border: 1px solid #555;
                padding: 5px;
                border-radius: 3px;
            }

            QPushButton:hover {
                background-color: #4a4a4a;
            }

            QPushButton:pressed {
                background-color: #2a2a2a;
            }

            QLabel {
                color: #e0e0e0;
            }

            QScrollArea {
                background-color: #2d2d2d;
                border: none;
            }

            QGroupBox {
                background-color: #353535;
                color: #e0e0e0;
                border: 1px solid #444;
                border-radius: 5px;
                margin-top: 10px;
            }

            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 3px;
            }

            QTableWidget {
                background-color: #353535;
                color: #e0e0e0;
                gridline-color: #444;
            }

            QHeaderView::section {
                background-color: #3a3a3a;
                color: #e0e0e0;
                padding: 5px;
                border: 1px solid #444;
            }

            QTableWidget::item {
                padding: 5px;
            }

            QTableWidget::item:selected {
                background-color: #505050;
                color: #ffffff;
            }

            QLineEdit, QComboBox, QDateEdit {
                background-color: #3a3a3a;
                color: #e0e0e0;
                border: 1px solid #555;
                padding: 3px;
            }

            QComboBox QAbstractItemView {
                background-color: #3a3a3a;
                color: #e0e0e0;
            }

            QListWidget {
                background-color: #353535;
                color: #e0e0e0;
                border: 1px solid #444;
            }

            QListWidget::item:selected {
                background-color: #505050;
            }

            QMenuBar {
                background-color: #353535;
                color: #e0e0e0;
            }

            QMenuBar::item:selected {
                background-color: #505050;
            }

            QMenu {
                background-color: #353535;
                color: #e0e0e0;
                border: 1px solid #444;
            }

            QMenu::item:selected {
                background-color: #505050;
            }

            QMessageBox {
                background-color: #353535;
            }

            QInputDialog {
                background-color: #353535;
            }

            QTabWidget::pane {
                border: 1px solid #444;
                background: #353535;
            }

            QTabBar::tab {
                background: #3a3a3a;
                color: #e0e0e0;
                padding: 5px;
                border: 1px solid #444;
                border-bottom: none;
                border-top-left-radius: 3px;
                border-top-right-radius: 3px;
            }

            QTabBar::tab:selected {
                background: #505050;
                border-bottom-color: #505050;
            }

            QScrollBar:vertical {
                background: #3a3a3a;
                width: 12px;
            }

            QScrollBar::handle:vertical {
                background: #555;
                min-height: 20px;
            }

            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                background: none;
            }

            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
                background: none;
            }
        )";
    } else {
        currentThemeStyle = R"(
            /* Light Theme */
            QWidget {
                background-color: #f0f0f0;
                color: #333333;
                border: none;
            }

            QMainWindow, QDialog {
                background-color: #f0f0f0;
            }

            QPushButton {
                background-color: #e0e0e0;
                color: #333333;
                border: 1px solid #aaa;
                padding: 5px;
                border-radius: 3px;
            }

            QPushButton:hover {
                background-color: #d0d0d0;
            }

            QPushButton:pressed {
                background-color: #c0c0c0;
            }

            QLabel {
                color: #333333;
            }

            QScrollArea {
                background-color: #f0f0f0;
                border: none;
            }

            QGroupBox {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #ccc;
                border-radius: 5px;
                margin-top: 10px;
            }

            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 3px;
            }

            QTableWidget {
                background-color: #ffffff;
                color: #333333;
                gridline-color: #ddd;
            }

            QHeaderView::section {
                background-color: #e0e0e0;
                color: #333333;
                padding: 5px;
                border: 1px solid #ccc;
            }

            QTableWidget::item {
                padding: 5px;
            }

            QTableWidget::item:selected {
                background-color: #0078d7;
                color: #ffffff;
            }

            QLineEdit, QComboBox, QDateEdit {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #aaa;
                padding: 3px;
            }

            QComboBox QAbstractItemView {
                background-color: #ffffff;
                color: #333333;
            }

            QListWidget {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #ccc;
            }

            QListWidget::item:selected {
                background-color: #0078d7;
                color: #ffffff;
            }

            QMenuBar {
                background-color: #f0f0f0;
                color: #333333;
            }

            QMenuBar::item:selected {
                background-color: #e0e0e0;
            }

            QMenu {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #ccc;
            }

            QMenu::item:selected {
                background-color: #0078d7;
                color: #ffffff;
            }

            QMessageBox {
                background-color: #ffffff;
            }

            QInputDialog {
                background-color: #ffffff;
            }

            QTabWidget::pane {
                border: 1px solid #ccc;
                background: #ffffff;
            }

            QTabBar::tab {
                background: #e0e0e0;
                color: #333333;
                padding: 5px;
                border: 1px solid #ccc;
                border-bottom: none;
                border-top-left-radius: 3px;
                border-top-right-radius: 3px;
            }

            QTabBar::tab:selected {
                background: #ffffff;
                border-bottom-color: #ffffff;
            }

            QScrollBar:vertical {
                background: #f0f0f0;
                width: 12px;
            }

            QScrollBar::handle:vertical {
                background: #c0c0c0;
                min-height: 20px;
            }

            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                background: none;
            }

            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
                background: none;
            }
        )";
    }

    this->setStyleSheet(currentThemeStyle);

    // Обновление иконок в зависимости от темы
    QString iconColor = dark ? "white" : "black";
}
