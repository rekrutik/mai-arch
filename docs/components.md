# Компонентная архитектура

## Компонентная диаграмма
```plantuml
@startuml
!include https://raw.githubusercontent.com/plantuml-stdlib/C4-PlantUML/master/C4_Container.puml

AddElementTag("microService", $shape=EightSidedShape(), $bgColor="CornflowerBlue", $fontColor="white", $legendText="microservice")
AddElementTag("storage", $shape=RoundedBoxShape(), $bgColor="lightSkyBlue", $fontColor="white")

Person(admin, "Администратор")
Person(user, "Пользователь")

System_Ext(mobile, "Мобильное приложение", "Flutter, Swift, Kotlin")

System_Boundary(conference_site, "Сайт заказа услуг") {
   Container(auth_service, "Сервис регистрации и авторизации", "C++", "Сервис для создания и авторизации пользователей", $tags = "microService")
   Container(user_service, "Сервис пользователей", "C++", "Сервис управления, редактирования и получения информации о пользователях", $tags = "microService")    
   Container(product_service, "Сервис лотов услуг", "C++", "Сервис для создание/просмотра/редактирования предоставляемых услуг", $tags = "microService")
   Container(proxysql, "proxysql", "Proxy")
   ContainerDb(db0, "mariadb0", "MySQL", "Хранение данных о пользователях, аккаунтах и услугах", $tags = "storage")
   ContainerDb(db1, "mariadb1", "MySQL", "Хранение данных о пользователях", $tags = "storage")
   ContainerDb(db2, "mariadb02", "MySQL", "Хранение данных о пользователях", $tags = "storage")
}

Rel(admin, mobile, "Просмотр, управление пользователями и услугами")
Rel(user, mobile, "Регистрация; создание, отправка заявок на приобретение услуги")

Rel(mobile, auth_service, "Авторизация", "localhost/auth")
Rel(mobile, user_service, "Регистрация", "localhost/auth")
Rel(auth_service, proxysql, "INSERT/SELECT/UPDATE", "SQL")
Rel(mobile, user_service, "Работа с пользователями", "localhost/user")
Rel(user_service, proxysql, "INSERT/SELECT/UPDATE", "SQL")
Rel(mobile, product_service, "Работа с услугами", "localhost/order")
Rel(product_service, proxysql, "INSERT/SELECT/UPDATE", "SQL")

Rel(proxysql, db0, "INSERT/SELECT/UPDATE")
Rel(proxysql, db1, "INSERT/SELECT/UPDATE")
Rel(proxysql, db2, "SELECT/SELECT/UPDATE")

Rel(user_service, auth_service, "Валидация токена")
Rel(user_service, auth_service, "Создание аккаунта")
Rel(product_service, auth_service, "Валидация токена")

@enduml
```

# Список компонентов
Для обращения к компонентам, требующим авторизации, пользователь предоставляет Bearer Token в заголовке `Authorization`

## Сервис аутентификации
**REST:**
- Регистрация пользователя
   - Метод: POST
   - Параметры запроса: логин, пароль
   - Тело ответа: ID созданного пользователя
   - Код успешного завершения: 201
- Авторизация пользователя
   - Метод: GET
   - Авторизация через X-API-AUTH
   - Тело ответа: Bearer Token
   - Код успешного завершения: 200
- Валидация токена
   - Метод: GET
   - Авторизация через X-API-AUTH
   - Параметры запроса: логин, пароль
   - Тело ответа: Bearer Token
   - Код успешного завершения: 200

## Сервис управления пользователями
**REST:**
- Создание пользователя
   - Метод: PUT
   - Параметры запроса: логин, имя, пароль
   - Тело ответа: Bearer Token
   - Код успешного завершения: 201
- Поиск пользователя по ID
   - Метод: GET
   - Параметры запроса: ID пользователя
   - Тело ответа: модель пользователя
   - Код ответа: 200, если найден, 404 иначе
- Поиск пользователя по маске имени пользователя
   - Метод: GET
   - Параметры запроса: маска имени пользователя
   - Тело ответа: список моделей пользователя
   - Код ответа: 200
- Удаление пользователя
   - Метод - DELETE
   - Параметры запроса: ID пользователя
   - Код ответа: 204, если удален, 403 при отсутствии прав

## Сервиc услуг
**REST:**
- Создание новой услуги
   - Метод - PUT
   - Параметры запроса: название услуги, описание, цена
   - Код ответа: 201
- Получение услуги
   - Метод: GET
   - Параметры запроса: ID услуги
   - Тело ответа: модель услуги
   - Код ответа: 200, если найдено, 404 иначе
- Получение списка услуг
   - Метод: GET
   - Тело ответа: модели услуги
   - Код ответа: 200
- Создание заказа на услугу
   - Метод: PUT
   - Тело ответа: ID услуги
   - Код ответа: 201
- Создание списка заказанных услуг для данного пользователя
   - Метод: GET
   - Тело ответа: модели услуги
   - Код ответа: 200