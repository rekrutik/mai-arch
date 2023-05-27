CREATE TABLE IF NOT EXISTS `accounts` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `username` VARCHAR(128) NOT NULL UNIQUE,
    `password` VARCHAR(72) NOT NULL
);

CREATE TABLE IF NOT EXISTS `users` (
    `id` BIGINT NOT NULL PRIMARY KEY REFERENCES accounts(id),
    `name` VARCHAR(128) NOT NULL,
    `is_admin` BOOLEAN NOT NULL DEFAULT 0,
    `is_deleted` BOOLEAN NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS `services` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `name` VARCHAR(128) NOT NULL,
    `description` TEXT NOT NULL,
    `price` BIGINT NOT NULL,
    `seller_id` BIGINT NOT NULL,
    `created_at` DATETIME NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS `orders` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `buyer_id` BIGINT NOT NULL REFERENCES users(id),
    `service_id` BIGINT NOT NULL REFERENCES services(id),
    `created_at` DATETIME NOT NULL DEFAULT NOW(),
    KEY `buyer_service_index` ( `buyer_id`, `service_id` )
);

GRANT ALL PRIVILEGES ON db.* TO 'example-user';
