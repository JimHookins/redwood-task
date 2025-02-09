#ifndef BOARDMODEL_H
#define BOARDMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#define BOARD_SIZE      9
#define BALLS_TO_ADD    3

struct Ball {
    int color = 0; // 0 - empty, 1-4 - colored ball
    bool moved = false; // false - new, true - moved(not new)
};

class BoardModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
public:
    enum DataRoles {
        ColorRole = Qt::UserRole + 1,
        MovedRole,
    };
    enum BoardConstants {
        BoardSize = 9,
        BallsToAdd = 3
    };
    Q_ENUM(BoardConstants);

    explicit BoardModel(int size = BoardSize, QObject *parent = nullptr);
    ~BoardModel();
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Q_INVOKABLE void newGame();
    Q_INVOKABLE int size();
    Q_INVOKABLE int startBallMoving(int row, int column, int rowTo, int columnTo);
    Q_INVOKABLE void finishBallMoving(int row, int column, int color);
    void checkForLines();
    int score() const { return m_score; }
    Q_INVOKABLE void clearBoard();

signals:
    void scoreChanged();
    void gameOver();

private:
    int m_size;
    QVector<QVector<Ball>> board;
    QSqlDatabase db;
    int m_score;
    void addRandomBalls(int count);
    bool isGameOver() const;
    void initDB();
    void createTable();
    void loadState();
    void saveState();
    void insertBallToDB(int row, int col, int color);
    void removeBallFromDB(int row, int col);
};


#endif // BOARDMODEL_H
