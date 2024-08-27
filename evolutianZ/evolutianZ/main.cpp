#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <thread>

//using namespace sf;

// Функция для анимации движения бокового меню
void animateSideMenu(tgui::Panel::Ptr sideMenu, float targetX, float duration)
{
    // Получаем текущее время анимации
    sf::Clock clock;
    float startX = sideMenu->getPosition().x; // Начальная позиция
    float deltaX = targetX - startX; // Разница между текущей и целевой позициями

    // Цикл анимации
    while (clock.getElapsedTime().asSeconds() < duration)
    {
        // Рассчитываем текущее смещение
        float progress = clock.getElapsedTime().asSeconds() / duration;
        float newX = startX + deltaX * progress;
        sideMenu->setPosition(newX, sideMenu->getPosition().y);

        // Ожидаем обновления интерфейса (этот вызов можно заменить на ваш метод обновления)
        sf::sleep(sf::milliseconds(10));
    }

    // Устанавливаем панель в конечное положение
    sideMenu->setPosition(targetX, sideMenu->getPosition().y);
}

int main()
{
    const int width = 100; //ширина карты
    const int height = 50; //высота карты
    const int sizeTile = 10; //размер каждого тайла
    const float delay = 0.2f; //задержка нажатия
    int world[width][height] = {0}; //двумерный массив с клетками в этом ходе
    int nextStep[width][height] = {0}; //двумерный массив с клетками которые будут в следующем ходе
    int neighbours; //счетчик отвечающий за количество соседей
    float interval = 0;
    int mood = 1; //режим рисования
    float zoomLevel = 1.0f; //начальный зум
    int menuWidth = 200;
    bool menuVisible = false;

    /// |------------Палитра------------|
    /// |-------------------------------|
    /// | Белый - Базовая клетка        |
    /// | Зеленый - Лист(фотосинтез)    |
    /// | Коричневый - Корень(биомасса) |
    /// | Синий - Энергия               |
    /// |-------------------------------|

    /// black
    sf::RectangleShape tileBlack(sf::Vector2f(sizeTile, sizeTile));
    tileBlack.setFillColor(sf::Color::Black);

    /// green
    sf::RectangleShape tileGreen(sf::Vector2f(sizeTile, sizeTile));
    tileGreen.setFillColor(sf::Color::Green);

    /// brown
    sf::RectangleShape tileBrown(sf::Vector2f(sizeTile, sizeTile));
    tileBrown.setFillColor(sf::Color(115, 66, 34));

    /// blue
    sf::RectangleShape tileBlue(sf::Vector2f(sizeTile, sizeTile));
    tileBlue.setFillColor(sf::Color::Blue);

    sf::RenderWindow window(sf::VideoMode(width*sizeTile, height*sizeTile), "Evoluatian test");
    sf::View view(sf::FloatRect(0, 0, width*sizeTile, height*sizeTile));

    tgui::Gui gui(window);

    auto sideMenu = tgui::Panel::create({"200px", "100%"});
    sideMenu->setPosition(-200, 0);
    sideMenu->getRenderer()->setBackgroundColor(tgui::Color::Green);
    gui.add(sideMenu);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")){return -1;}

    sf::Clock clock; float time;

    while (window.isOpen())
    {
        time = clock.getElapsedTime().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // Обработка событий масштабирования
            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.delta > 0) {
                    zoomLevel *= 0.9f; // Приближение
                } else {
                    zoomLevel *= 1.1f; // Отдаление
                }
                view.setSize(width*sizeTile * zoomLevel, height*sizeTile * zoomLevel);
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::M)
            {
                // Если нажата клавиша "M", переключаем видимость бокового меню с анимацией
                if (!menuVisible) {
                    std::thread(animateSideMenu, sideMenu, 0.0f, 0.5f).detach(); // Показать меню за 0.5 секунд
                } else {
                    std::thread(animateSideMenu, sideMenu, -200.0f, 0.5f).detach(); // Скрыть меню за 0.5 секунд
                }
                menuVisible = !menuVisible; // Инвертируем состояние меню
            }
        }

        window.clear(sf::Color::White);

        // Рисуем вертикальные линии
        for (int x = 0; x < width*sizeTile + sizeTile; x += sizeTile) {
            sf::VertexArray line(sf::Lines, 2);
            line[0].position = sf::Vector2f(x, 0);
            line[1].position = sf::Vector2f(x, height*sizeTile);
            line[0].color = sf::Color::Black;
            line[1].color = sf::Color::Black;
            window.draw(line);
        }

        // Рисуем горизонтальные линии
        for (int y = 0; y < height*sizeTile + sizeTile; y += sizeTile) {
            sf::VertexArray line(sf::Lines, 2);
            line[0].position = sf::Vector2f(0, y);
            line[1].position = sf::Vector2f(width*sizeTile, y);
            line[0].color = sf::Color::Black;
            line[1].color = sf::Color::Black;
            window.draw(line);
        }
        //window.display();
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window); //позиция мышки относительно окны
        sf::Vector2f pos = window.mapPixelToCoords(pixelPos);

        /// |---------------------Управление-------------------|
        /// |--------------------------------------------------|
        /// | ЛКМ - Рисовать клетками                          |
        /// | ПКМ - Стирать клетки                             |
        /// | 1,2,3,4 - Смена режима клетки                    |
        /// | Л. альт - Смена режима на следущий               |
        /// | Escape - Очистка поля                            |
        /// | W, A, S, D / Up, Left, Right, Down - Перемещение |
        /// | Mouse Wheel Up/Down - зумирование                |
        /// |--------------------------------------------------|

        // Обработка событий перемещения
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            view.move(0, -10 * zoomLevel); // Вверх
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            view.move(0, 10 * zoomLevel); // Вниз
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            view.move(-10 * zoomLevel, 0); // Влево
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            view.move(10 * zoomLevel, 0); // Вправо
        }

        // Устанавливаем вид
        window.setView(view);

        // Рисование / Стирание
        if(sf::Mouse::isButtonPressed(sf::Mouse::Left) && pos.x < width * sizeTile){ // ЛКМ
            int cellX = static_cast<int>(pos.x / sizeTile);
            int cellY = static_cast<int>(pos.y / sizeTile);
            if(cellX >= 0 && cellX < (width*sizeTile) / sizeTile && cellY >= 0 && cellY < height) {
                world[cellX][cellY] = mood;
                nextStep[cellX][cellY] = mood;
            }

            //world[int(pos.x/sizeTile)][int(pos.y/sizeTile)] = mood;
            //nextStep[int(pos.x/sizeTile)][int(pos.y/sizeTile)] = mood;
            }
        if(sf::Mouse::isButtonPressed(sf::Mouse::Right) && pos.x < width * sizeTile){ // ПКМ
            int cellX = static_cast<int>(pos.x / sizeTile);
            int cellY = static_cast<int>(pos.y / sizeTile);
            if(cellX >= 0 && cellX < (width*sizeTile) / sizeTile && cellY >= 0 && cellY < height) {
                world[cellX][cellY] = 0;
                nextStep[cellX][cellY] = 0;
            }

            //world[int(pos.x/sizeTile)][int(pos.y/sizeTile)] = 0;
            //nextStep[int(pos.x/sizeTile)][int(pos.y/sizeTile)] = 0;
            }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){ // Escape
            for(int i = 0; i < width; i++){
                for(int j = 0; j < height; j++){
                    nextStep[i][j] = 0;
                    world[i][j] = 0;
                }
            }
        }

        // Обработка режимов
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)){ // 1
            if(clock.getElapsedTime().asSeconds() >= delay){
                mood = 1;
                clock.restart();
            }
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)){ // 2
            if(clock.getElapsedTime().asSeconds() >= delay){
                mood = 2;
                clock.restart();
            }
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)){ // 3
            if(clock.getElapsedTime().asSeconds() >= delay){
                mood = 3;
                clock.restart();
            }
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)){ // 4
            if(clock.getElapsedTime().asSeconds() >= delay){
                mood = 4;
                clock.restart();
            }
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)){ // Левый альт
            if(clock.getElapsedTime().asSeconds() >= delay){
                if(mood == 4){mood = 1;}
                else{mood++;}
                clock.restart();
            }
        }

        // Следущий шаг
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && interval <= time){
            clock.restart();
            for(int i = 0; i < width; i++){
                for(int j = 0; j < height; j++){ //в переменную neighbours складываем 8 соседских клеток(0 или 1), (i+width+1)%width а не просто i+1 потому что карта замкнутая
                    //neighbours = world[(i+width-1)%width][j] + world[(i+width+1)%width][j] + world[i][(j+height-1)%height] + world[i][(j+height+1)%height];
                    int neighbours = 0;
                    if(world[(i+width-1)%width][j] != 0){neighbours+=1;}
                    if(world[(i+width+1)%width][j] != 0){neighbours+=1;}
                    if(world[i][(j+height-1)%height] != 0){neighbours+=1;}
                    if(world[i][(j+height+1)%height] != 0){neighbours+=1;}
                    if(world[(i+width-1)%width][(j+height-1)%height] != 0){neighbours+=1;}
                    if(world[(i+width+1)%width][(j+height-1)%height] != 0){neighbours+=1;}
                    if(world[(i+width-1)%width][(j+height+1)%height] != 0){neighbours+=1;}
                    if(world[(i+width+1)%width][(j+height+1)%height] != 0){neighbours+=1;}
                    //std::cout << neighbours << std::endl;

                    if(world[i][j] != 0){ //если в текущем поколений клетка жива

                    }
                    else { //иначе

                    }
                }
            }
            for(int i = 0; i < width; i++){
                for(int j = 0; j < height; j++){
                    world[i][j] = nextStep[i][j]; //копируем массив nextStep в world
                }
            }
        }

        //window.clear(sf::Color::White); //очищаем окно в белый цвет

        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                    if(world[i][j] == 1){
                        tileBlack.setPosition(sf::Vector2f(i*sizeTile, j*sizeTile));
                        window.draw(tileBlack);
                    }
                    else if(world[i][j] == 2){
                        tileGreen.setPosition(sf::Vector2f(i*sizeTile, j*sizeTile));
                        window.draw(tileGreen);
                    }
                    else if(world[i][j] == 3){
                        tileBrown.setPosition(sf::Vector2f(i*sizeTile, j*sizeTile));
                        window.draw(tileBrown);
                    }
                    else if(world[i][j] == 4){
                        tileBlue.setPosition(sf::Vector2f(i*sizeTile, j*sizeTile));
                        window.draw(tileBlue);
                    }
            }
        }
        gui.draw();
        window.display();
    }

    return 0;
}
