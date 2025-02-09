#include "boardmodel.h"
#include <QDebug>

BoardModel::BoardModel(int size, QObject *parent)
    :
    QAbstractTableModel(parent),
    m_size(size),
    board(size, QVector<Ball>(size, {0})),
    m_score(0)
{
    initDB();
    loadState();
}

BoardModel::~BoardModel()
{
    saveState();
}

QHash<int, QByteArray> BoardModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ColorRole] = "colorRole";
    roles[MovedRole] = "isMoved";
    return roles;
}

int BoardModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    return m_size;
}

int BoardModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    return m_size;
}

QVariant BoardModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();
    if (role == ColorRole)
        return board[index.row()][index.column()].color;
    if (role == MovedRole)
        return board[index.row()][index.column()].moved;
    return QVariant();
}

bool BoardModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || role != Qt::EditRole)
        return false;
    board[index.row()][index.column()].color = value.toInt();
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags BoardModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void BoardModel::initDB() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("game_state.db");
    if (!db.open()) {
        qDebug() << "Database open error:" << db.lastError().text();
    }
    createTable();
}

void BoardModel::createTable() {
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS board (row INTEGER, col INTEGER, color INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS game_state (score INTEGER)");

    // Ensure there's only one score record
    query.exec("INSERT INTO game_state (score) SELECT 0 WHERE NOT EXISTS (SELECT 1 FROM game_state)");

}

void BoardModel::insertBallToDB(int row, int col, int color) {
    QSqlQuery query;
    query.prepare("INSERT INTO board (row, col, color) VALUES (?, ?, ?)");
    query.addBindValue(row);
    query.addBindValue(col);
    query.addBindValue(color);
    if (!query.exec()) {
        qDebug() << "Insert error:" << query.lastError().text();
    }
}

void BoardModel::removeBallFromDB(int row, int col) {
    QSqlQuery query;
    query.prepare("DELETE FROM board WHERE row = ? AND col = ?");
    query.addBindValue(row);
    query.addBindValue(col);

    if (!query.exec()) {
        qDebug() << "Delete error:" << query.lastError().text();
    }
}


void BoardModel::clearBoard() {
    QSqlQuery query;
    query.exec("DELETE FROM board");
    board.fill(QVector<Ball>(BOARD_SIZE, {0}), BOARD_SIZE);
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
}

void BoardModel::loadState() {
    // Load board state
    QSqlQuery query("SELECT row, col, color FROM board");
    board.fill(QVector<Ball>(BOARD_SIZE, {0}), BOARD_SIZE);
    while (query.next()) {
        int row = query.value(0).toInt();
        int col = query.value(1).toInt();
        int color = query.value(2).toInt();
        board[row][col].color = color;
        board[row][col].moved = true;
    }

    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));

    // Load score
    query.exec("SELECT score FROM game_state");
    if (query.next()) {
        m_score = query.value(0).toInt();
    } else {
        m_score = 0;
    }

    emit scoreChanged();
}

void BoardModel::saveState() {
    // Save board state
    QSqlQuery query;
    query.prepare("INSERT INTO board (row, col, color) VALUES (?, ?, ?)");
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (board[row][col].color != 0) {
                query.addBindValue(row);
                query.addBindValue(col);
                query.addBindValue(board[row][col].color);
                query.exec();
            }
        }
    }

    // Save score
    query.exec("DELETE FROM game_state");  // clear the previous one
    query.prepare("INSERT INTO game_state (score) VALUES (?)");
    query.addBindValue(m_score);
    query.exec();
}

void BoardModel::addRandomBalls(int count) {
    qDebug() << "addRandomBalls";
    srand(time(nullptr));
    int attempts = 0;
    int maxAttempts = rowCount() * columnCount();
    for (int i = 0; i < count; ++i) {
        int row, col;
        do {
            row = rand() % BOARD_SIZE;
            col = rand() % BOARD_SIZE;
            attempts++;

            if (attempts > maxAttempts)
                break;
        } while (board[row][col].color != 0);

        int color = (rand() % 4) + 1;
        board[row][col].color = color;
        //insertBall(row, col, color);// not necessary, will be saved in destructor
    }

    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));

    // Check for lines as well?
    //checkForLines();

    if (isGameOver()) {
        qDebug() << "Game over!";
        emit gameOver();
    }
}

bool BoardModel::isGameOver() const {
    // all cells are occupied?
    for (const auto &row : board) {
        for (const auto &ball : row) {
            if (ball.color == 0) {
                return false;
            }
        }
    }
    return true;
}

void BoardModel::newGame() {
    clearBoard();
    m_score = 0;
    emit scoreChanged();
    addRandomBalls(BoardConstants::BallsToAdd);
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
}

int BoardModel::size()
{
    return m_size;
}

int BoardModel::startBallMoving(int row, int column, int rowTo, int columnTo)
{
    qDebug() << "startBallMoving" << row << column << rowTo << columnTo;
    if (board[rowTo][columnTo].color != 0) return 0;

    int color = board[row][column].color;
    board[row][column].color = 0;
    board[row][column].moved = true;
    removeBallFromDB(row, column);

    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));

    return color;
}

void BoardModel::finishBallMoving(int row, int column, int color)
{
    qDebug() << "finishBallMoving" << row << column << color;

    board[row][column].color = color;
    board[row][column].moved = true;

    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));

    checkForLines();
    addRandomBalls(BALLS_TO_ADD);
}

void BoardModel::checkForLines() {
    qDebug() << "checkForLines";
    QList<QPair<int, int>> ballsToRemove;

    // Directions: Horizontal and Vertical
    const QVector<QPair<int, int>> directions = {
        {0, 1},   // Horizontal
        {1, 0}    // Vertical
    };

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            int color = board[row][col].color;
            if (color == 0) continue;  // skip empty cells

            for (auto [rowStep, colStep] : directions) {
                QList<QPair<int, int>> line;

                // Traverse in the current direction
                int r = row, c = col;
                while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE &&
                       board[r][c].color == color) {
                    line.append({r, c});
                    r += rowStep;
                    c += colStep;
                }

                // Remove only if the line has 5 or more balls
                if (line.size() >= 5) {
                    for (auto pos : line) {
                        ballsToRemove.append(pos);
                    }
                }
            }
        }
    }

    // Remove balls and update score
    if (!ballsToRemove.isEmpty()) {
        QSet<QPair<int, int>> uniqueBalls(ballsToRemove.begin(), ballsToRemove.end());
        for (const auto& pos : uniqueBalls) {
            board[pos.first][pos.second].color = 0;
            removeBallFromDB(pos.first, pos.second);
        }
        m_score += uniqueBalls.size();
        emit scoreChanged();
        emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
    }
}

