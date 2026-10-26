-- =============================================================================
-- OmniCore.sql - Master Platform Schema DDL & Data Migration Script
-- Strict PostgreSQL Case Preservation with Double Quotes
-- Modules: Identity, Users, Sessions, GlobalConfiguration, SystemConfig,
--          Venues, Events, DeparturePoints, Routes, RouteStops, Schedules,
--          Tickets, NotificationContacts, NotificationSettings, Reservations,
--          Multi-Gateway Payments (Stripe, OpenPay, MercadoPago),
--          WhatsApp Cloud API & Custom Notification Templates
-- =============================================================================

-- 1. Custom ENUM Types Check
DO $$ 
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PermissionMode') THEN
        CREATE TYPE "PermissionMode" AS ENUM ('P', 'R');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PaymentMethodType') THEN
        CREATE TYPE "PaymentMethodType" AS ENUM (
            'NOT_APPLICABLE',
            'CASH',
            'TRANSFER',
            'CARD',
            'OTHER'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PaymentStatusType') THEN
        CREATE TYPE "PaymentStatusType" AS ENUM (
            'UNPAID',
            'PENDING',
            'PAID',
            'REFUNDED',
            'CANCELLED'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'DeparturePointType') THEN
        CREATE TYPE "DeparturePointType" AS ENUM (
            'PICKUP',
            'INTERMEDIATE',
            'DROPOFF'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'ScheduleType') THEN
        CREATE TYPE "ScheduleType" AS ENUM (
            'DEPARTURE',
            'RETURN',
            'ROUND_TRIP'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'EventCategory') THEN
        CREATE TYPE "EventCategory" AS ENUM (
            'MUSIC',
            'SPORTS',
            'THEATER',
            'FESTIVAL',
            'CULTURE',
            'OTHER'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'ReservationStatusType') THEN
        CREATE TYPE "ReservationStatusType" AS ENUM (
            'PENDING',
            'UNCONFIRMED',
            'CONFIRMED',
            'CANCELLED',
            'COMPLETED'
        );
    END IF;

    -- Migration: add PENDING to ReservationStatusType if missing
    IF EXISTS (SELECT 1 FROM pg_type WHERE typname = 'ReservationStatusType') THEN
        IF NOT EXISTS (SELECT 1 FROM pg_enum WHERE enumlabel = 'PENDING' AND enumtypid = (SELECT oid FROM pg_type WHERE typname = 'ReservationStatusType')) THEN
            ALTER TYPE "ReservationStatusType" ADD VALUE IF NOT EXISTS 'PENDING' BEFORE 'UNCONFIRMED';
        END IF;
    END IF;
END $$;

-- 2. Users
CREATE TABLE IF NOT EXISTS "Users" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Email" VARCHAR(50),
    "Phone" VARCHAR(50),
    "Employee" INT,
    "EmployeeCode" VARCHAR(50),
    "Department" INT,
    "PermissionMode" "PermissionMode" NOT NULL DEFAULT 'P',
    "RoleEntry" INT,
    "RoleCode" VARCHAR(50),
    "MaxDisccountPerLine" NUMERIC(12, 6),
    "MaxDisccountPerDocument" NUMERIC(12, 6),
    "SuperUser" BOOLEAN NOT NULL DEFAULT false,
    "IsLocked" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "Password" BYTEA,
    "ChangePasswordNextLogin" BOOLEAN NOT NULL DEFAULT false,
    "PasswordNeverExpires" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_Users_EmployeeCode" ON "Users" ("EmployeeCode");
CREATE INDEX IF NOT EXISTS "IDX_Users_RoleCode" ON "Users" ("RoleCode");

-- Default System & Administrator SuperUsers
INSERT INTO "Users" (
    "Code", "Name", "Email", "SuperUser", "IsLocked", "IsActive",
    "ChangePasswordNextLogin", "PasswordNeverExpires", "CreatedBy"
) VALUES 
('system', 'Administrador General del Sistema', 'system@omnisphere.local', true, false, true, false, true, 'system'),
('admin', 'System Administrator', 'admin@omnisphere.local', true, false, true, false, true, 'system')
ON CONFLICT ("Name") DO NOTHING;

-- 2.1 Employees (Agnostic Master Entity)
CREATE TABLE IF NOT EXISTS "Employees" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "FirstName" VARCHAR(100),
    "SecondName" VARCHAR(100),
    "LastName" VARCHAR(100),
    "SecondLastName" VARCHAR(100),
    "Email" VARCHAR(100),
    "Phone" VARCHAR(50),
    "Department" VARCHAR(100),
    "Position" VARCHAR(100),
    "DirectManagerCode" VARCHAR(50),
    "DateOfBirth" DATE,
    "Comments" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Employees_Code_Active" ON "Employees" ("Code") WHERE "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Employees_Email" ON "Employees" ("Email");

-- 3. Sessions
CREATE TABLE IF NOT EXISTS "Sessions" (
    "SessionEntry" SERIAL PRIMARY KEY,
    "SessionUUID" VARCHAR(100) NOT NULL UNIQUE DEFAULT gen_random_uuid()::text,
    "UserCode" VARCHAR(50),
    "UserEmail" VARCHAR(50),
    "UserPhone" VARCHAR(50),
    "StartDate" VARCHAR(50),
    "EndDate" VARCHAR(50),
    "DurationSeconds" INT DEFAULT 0,
    "DeviceIP" VARCHAR(50),
    "HostName" VARCHAR(100),
    "IsActive" CHAR(1) NOT NULL DEFAULT 'Y',
    "Reason" VARCHAR(255),
    "LogoutMessage" VARCHAR(255),
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Sessions_SessionUUID" ON "Sessions" ("SessionUUID");
CREATE INDEX IF NOT EXISTS "IDX_Sessions_UserCode_IsActive" ON "Sessions" ("UserCode", "IsActive");

-- 4. SystemConfigs
CREATE TABLE IF NOT EXISTS "SystemConfigs" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "FeeHandlingStrategy" VARCHAR(50) NOT NULL DEFAULT 'SURCHARGE',
    "TaxRatePercent" NUMERIC(5, 2) NOT NULL DEFAULT 16.00,
    "DefaultCurrency" VARCHAR(10) NOT NULL DEFAULT 'MXN',
    "CompanyName" VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Enterprise',
    "EnableEmailNotifications" BOOLEAN NOT NULL DEFAULT true,
    "EnableWhatsappNotifications" BOOLEAN NOT NULL DEFAULT true,
    "AllowPartialPayments" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- 6. Identities (Centralized Unpadded Prefix & Sequence Management)
CREATE TABLE IF NOT EXISTS "Identities" (
    "Entry" SERIAL PRIMARY KEY,
    "Domain" VARCHAR(50) NOT NULL UNIQUE,
    "Prefix1" VARCHAR(3) NOT NULL,
    "Prefix2" VARCHAR(3),
    "Prefix3" VARCHAR(3),
    "CurrentSequence" INT NOT NULL DEFAULT 0,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Identities_IsActive" CHECK ("IsActive" IN (true, false)),
    CONSTRAINT "CHK_Identities_CurrentSequence" CHECK ("CurrentSequence" >= 0),
    CONSTRAINT "CHK_Identities_Prefix1_Len" CHECK (LENGTH(TRIM("Prefix1")) = 3)
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Identities_Domain_Active" ON "Identities" ("Domain") WHERE "IsActive" = true;

INSERT INTO "Identities" ("Domain", "Prefix1", "CurrentSequence") VALUES
('Venue', 'VNU', 0),
('Event', 'EVT', 0),
('DeparturePoint', 'DEP', 0),
('Route', 'RTE', 0),
('RouteStop', 'STP', 0),
('Schedule', 'SCH', 0),
('Ticket', 'TCK', 0),
('NotificationContact', 'NTC', 0),
('NotificationSetting', 'NTS', 0),
('Reservation', 'RSV', 0),
('PaymentMethod', 'PMT', 4),
('WhatsAppSettings', 'WAS', 0),
('WhatsAppConversation', 'WAC', 0),
('WhatsAppMessage', 'WAM', 0),
('CustomMessage', 'MSG', 0),
('CustomButton', 'BTN', 0),
('CustomAttachment', 'ATT', 0),
('PaymentTransaction', 'TXN', 0),
('StripeTransaction', 'STX', 0),
('TransferTransactions', 'TRF', 0),
('CashTransactions', 'CSH', 0),
('CardTransactions', 'CRD', 0),
('User', 'USR', 1),
('Employee', 'EMP', 0),
('Role', 'ROL', 1)
ON CONFLICT ("Domain") DO NOTHING;

-- 7. Venues
CREATE TABLE IF NOT EXISTS "Venues" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "City" VARCHAR(3) NOT NULL,
    "Address" TEXT NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Venues_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Venues_Name_Active" ON "Venues" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Venues_Code_Active" ON "Venues" ("Code") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 8. Events
CREATE TABLE IF NOT EXISTS "Events" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Date" TIMESTAMP NOT NULL,
    "VenueCode" VARCHAR(50) NOT NULL,
    "Image" TEXT,
    "Category" "EventCategory",
    "IsPromoted" BOOLEAN NOT NULL DEFAULT false,
    "IsUpcoming" BOOLEAN NOT NULL DEFAULT true,
    "CommingSoon" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Events_IsPromoted" CHECK ("IsPromoted" IN (true, false)),
    CONSTRAINT "CHK_Events_IsUpcoming" CHECK ("IsUpcoming" IN (true, false)),
    CONSTRAINT "CHK_Events_CommingSoon" CHECK ("CommingSoon" IN (true, false)),
    CONSTRAINT "CHK_Events_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Events_Name_Active" ON "Events" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Events_VenueCode_Active" ON "Events" ("VenueCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Events_Date_Active" ON "Events" ("Date") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Events_Category_Active" ON "Events" ("Category") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 9. DeparturePoints
CREATE TABLE IF NOT EXISTS "DeparturePoints" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "City" VARCHAR(3) NOT NULL,
    "Address" TEXT NOT NULL,
    "PointType" "DeparturePointType" NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_DeparturePoints_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_DeparturePoints_Name_Active" ON "DeparturePoints" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_DeparturePoints_City_PointType_Active" ON "DeparturePoints" ("City", "PointType") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 10. Routes
CREATE TABLE IF NOT EXISTS "Routes" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "OriginPointCode" VARCHAR(50) NOT NULL,
    "DestinationVenueCode" VARCHAR(50) NOT NULL,
    "BasePrice" NUMERIC(10, 2) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Routes_BasePrice" CHECK ("BasePrice" >= 0),
    CONSTRAINT "CHK_Routes_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Routes_Name_Active" ON "Routes" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Routes_Origin_Dest_Active" ON "Routes" ("OriginPointCode", "DestinationVenueCode") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 11. RouteStops
CREATE TABLE IF NOT EXISTS "RouteStops" (
    "Entry" SERIAL PRIMARY KEY,
    "RouteCode" VARCHAR(50) NOT NULL,
    "DeparturePointCode" VARCHAR(50),
    "VenueCode" VARCHAR(50),
    "Type" "DeparturePointType" NOT NULL,
    "BasePrice" NUMERIC(10, 2) NOT NULL,
    "ArrivalTime" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_RouteStops_BasePrice" CHECK ("BasePrice" >= 0),
    CONSTRAINT "CHK_RouteStops_IsActive" CHECK ("IsActive" IN (true, false)),
    CONSTRAINT "CHK_RouteStops_Target" CHECK (
        ("Type" IN ('PICKUP', 'INTERMEDIATE') AND "DeparturePointCode" IS NOT NULL) OR
        ("Type" = 'DROPOFF' AND ("VenueCode" IS NOT NULL OR "DeparturePointCode" IS NOT NULL))
    )
);

CREATE INDEX IF NOT EXISTS "IDX_RouteStops_RouteCode_Active" ON "RouteStops" ("RouteCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_RouteStops_DeparturePointCode" ON "RouteStops" ("DeparturePointCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_RouteStops_VenueCode" ON "RouteStops" ("VenueCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE UNIQUE INDEX IF NOT EXISTS "UQ_RouteStops_RouteCode_Pickup_Active" ON "RouteStops" ("RouteCode") WHERE "Type" = 'PICKUP' AND "IsActive" = true AND "IsCanceled" = false;
CREATE UNIQUE INDEX IF NOT EXISTS "UQ_RouteStops_RouteCode_Dropoff_Active" ON "RouteStops" ("RouteCode") WHERE "Type" = 'DROPOFF' AND "IsActive" = true AND "IsCanceled" = false;

-- 12. Schedules
CREATE TABLE IF NOT EXISTS "Schedules" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "EventCode" VARCHAR(50) NOT NULL,
    "RouteCode" VARCHAR(50),
    "Type" "ScheduleType" NOT NULL,
    "DepartureTime" TIMESTAMP NOT NULL,
    "DepartureLabel" VARCHAR(100) NOT NULL,
    "ArrivalTime" TIMESTAMP,
    "ReturnDepartureTime" TIMESTAMP,
    "ReturnArrivalTime" TIMESTAMP,
    "Duration" INT,
    "Capacity" INT NOT NULL,
    "AvailableSeats" INT NOT NULL,
    "Price" NUMERIC(10, 2) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Schedules_Capacity" CHECK ("Capacity" >= 0),
    CONSTRAINT "CHK_Schedules_AvailableSeats" CHECK ("AvailableSeats" >= 0),
    CONSTRAINT "CHK_Schedules_Price" CHECK ("Price" >= 0),
    CONSTRAINT "CHK_Schedules_AvailableSeats_Capacity" CHECK ("AvailableSeats" <= "Capacity"),
    CONSTRAINT "CHK_Schedules_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Schedules_EventCode_Type_Active" ON "Schedules" ("EventCode", "Type") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Schedules_RouteCode_Active" ON "Schedules" ("RouteCode") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 13. Tickets
CREATE TABLE IF NOT EXISTS "Tickets" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "ScheduleCode" VARCHAR(50) NOT NULL,
    "Phone" VARCHAR(50) NOT NULL,
    "Quantity" INT NOT NULL DEFAULT 1,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Tickets_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_Tickets_Quantity" CHECK ("Quantity" > 0),
    CONSTRAINT "CHK_Tickets_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Tickets_ScheduleCode_Active" ON "Tickets" ("ScheduleCode") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 14. NotificationContacts
CREATE TABLE IF NOT EXISTS "NotificationContacts" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Phone" VARCHAR(50) NOT NULL,
    "Role" VARCHAR(100) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_NotificationContacts_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_NotificationContacts_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_NotificationContacts_Phone_Active" ON "NotificationContacts" ("Phone") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 15. NotificationSettings
CREATE TABLE IF NOT EXISTS "NotificationSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Notification Settings',
    "OwnerWhatsapp" VARCHAR(50) NOT NULL,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_NotificationSettings_Code" CHECK (LENGTH(TRIM("Code")) > 0),
    CONSTRAINT "CHK_NotificationSettings_OwnerWhatsapp" CHECK (LENGTH(TRIM("OwnerWhatsapp")) >= 7)
);

-- 16. Reservations
CREATE TABLE IF NOT EXISTS "Reservations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "FirstName1" VARCHAR(100) NOT NULL,
    "FirstName2" VARCHAR(100),
    "LastName1" VARCHAR(100) NOT NULL,
    "LastName2" VARCHAR(100),
    "Email" VARCHAR(100),
    "Phone" VARCHAR(50) NOT NULL,
    "Seats" INT NOT NULL DEFAULT 1,
    "EventCode" VARCHAR(50) NOT NULL,
    "RouteCode" VARCHAR(50) NOT NULL,
    "PickupPointCode" VARCHAR(50) NOT NULL,
    "DropoffPointCode" VARCHAR(50) NOT NULL,
    "ScheduleCode" VARCHAR(50),
    "TripType" "ScheduleType" NOT NULL DEFAULT 'ROUND_TRIP',
    "UnitPrice" NUMERIC(10, 2) NOT NULL DEFAULT 0.00,
    "TotalPrice" NUMERIC(10, 2) NOT NULL DEFAULT 0.00,
    "PaymentMethod" VARCHAR(50) NOT NULL DEFAULT 'TRANSFER',
    "PaymentStatus" "PaymentStatusType" NOT NULL DEFAULT 'PENDING',
    "PaymentReference" VARCHAR(100),
    "PaymentDate" TIMESTAMP,
    "CancellationReason" TEXT,
    "CancelledDate" TIMESTAMP,
    "Status" "ReservationStatusType" NOT NULL DEFAULT 'UNCONFIRMED',
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Reservations_FirstName1" CHECK (LENGTH(TRIM("FirstName1")) > 0),
    CONSTRAINT "CHK_Reservations_LastName1" CHECK (LENGTH(TRIM("LastName1")) > 0),
    CONSTRAINT "CHK_Reservations_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_Reservations_Seats" CHECK ("Seats" > 0),
    CONSTRAINT "CHK_Reservations_UnitPrice" CHECK ("UnitPrice" >= 0),
    CONSTRAINT "CHK_Reservations_TotalPrice" CHECK ("TotalPrice" >= 0),
    CONSTRAINT "CHK_Reservations_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Reservations_EventCode_Active" ON "Reservations" ("EventCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_RouteCode_Active" ON "Reservations" ("RouteCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_Phone_Active" ON "Reservations" ("Phone") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_Status_Active" ON "Reservations" ("Status") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_PaymentStatus_Active" ON "Reservations" ("PaymentStatus") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_PaymentMethod_Active" ON "Reservations" ("PaymentMethod") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_ExpiresAt_Pending" ON "Reservations" ("ExpiresAt") WHERE "Status" = 'PENDING' AND "IsActive" = true;

-- 17. PaymentMethods
CREATE TABLE IF NOT EXISTS "PaymentMethods" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Type" "PaymentMethodType" NOT NULL DEFAULT 'NOT_APPLICABLE',
    "UsesCommission" BOOLEAN NOT NULL DEFAULT false,
    "CommissionRate" NUMERIC(5, 2) NOT NULL DEFAULT 0.00,
    "UsesIntegration" BOOLEAN NOT NULL DEFAULT false,
    "IntegrationProvider" VARCHAR(50),
    "WhatsAppTemplateCode" VARCHAR(100),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethods_IsActive" CHECK ("IsActive" IN (true, false))
);

ALTER TABLE "PaymentMethods" ADD COLUMN IF NOT EXISTS "WhatsAppTemplateCode" VARCHAR(100);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_PaymentMethods_Name_Active" ON "PaymentMethods" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;

INSERT INTO "PaymentMethods" ("Code", "Name", "Type", "UsesCommission", "CommissionRate", "CreatedBy") VALUES
('PMT1', 'Efectivo', 'CASH', false, 0.00, 'system'),
('PMT2', 'Transferencia', 'TRANSFER', false, 0.00, 'system'),
('PMT3', 'Tarjeta / Otro', 'CARD', true, 3.50, 'system'),
('PMT4', 'No Aplica / Pend.', 'NOT_APPLICABLE', false, 0.00, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 17.1 PaymentMethodDetails (Detalles bancarios para transferencias/sin integración)
CREATE TABLE IF NOT EXISTS "PaymentMethodDetails" (
    "Entry"            SERIAL PRIMARY KEY,
    "Code"             VARCHAR(50) NOT NULL,
    "BankName"         VARCHAR(255) NOT NULL,
    "CLABE"            VARCHAR(255) NOT NULL,
    "AccountHolder"    VARCHAR(255) NOT NULL,
    "PaymentReference" VARCHAR(255),
    "IsActive"         BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy"        VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate"       TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy"    VARCHAR(50),
    "UpdateDate"       TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethodDetails_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_PMDetails_Code" 
    ON "PaymentMethodDetails" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_PMDetails_Active" 
    ON "PaymentMethodDetails" ("Code", "IsActive") WHERE "IsActive" = true;

-- 18. PaymentTransactions (Auditoría Universal de Transacciones de Cobro)
CREATE TABLE IF NOT EXISTS "PaymentTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "EntityType" VARCHAR(50) NOT NULL DEFAULT 'ROUTE_RESERVATION',
    "EntityCode" VARCHAR(50) NOT NULL,
    "Provider" VARCHAR(50) NOT NULL DEFAULT 'STRIPE',
    "PaymentMethod" VARCHAR(50) NOT NULL DEFAULT 'CARD',
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'PENDING',
    "PaymentIntentId" VARCHAR(255),
    "SessionId" VARCHAR(255),
    "Clabe" VARCHAR(50),
    "BankName" VARCHAR(100),
    "ReceiptUrl" TEXT,
    "HostedUrl" TEXT,
    "RawPayload" TEXT,
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Entity" ON "PaymentTransactions" ("EntityType", "EntityCode");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Status" ON "PaymentTransactions" ("Status");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_ExpiresAt_Pending" ON "PaymentTransactions" ("ExpiresAt") WHERE "Status" = 'PENDING' AND "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_PaymentIntentId" ON "PaymentTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Clabe" ON "PaymentTransactions" ("Clabe");

-- 18.1 Integrations (Platform Master Catalog of Integrations)
CREATE TABLE IF NOT EXISTS "Integrations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL,
    "DisplayName" VARCHAR(255) NOT NULL,
    "Category" VARCHAR(50) NOT NULL DEFAULT 'PAYMENT',
    "SettingsTable" VARCHAR(100),
    "LicenseModule" VARCHAR(50),
    "RequiredColumns" VARCHAR(255),
    "Description" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "Integrations" ("Code", "Name", "DisplayName", "Category", "SettingsTable", "LicenseModule", "RequiredColumns", "Description", "IsActive")
VALUES
    ('STRIPE', 'Stripe', 'Stripe Payments (Tarjeta / SPEI)', 'PAYMENT', 'StripeSettings', 'MODULE_STRIPE', 'PublishableKey,SecretKey', 'Procesamiento de pagos con tarjeta de crédito/débito y transferencias bancarias SPEI.', true),
    ('MERCADOPAGO', 'Mercado Pago', 'Mercado Pago (Tarjeta / Wallet)', 'PAYMENT', 'MercadoPagoSettings', 'MODULE_MERCADOPAGO', 'PublicKey,AccessToken', 'Plataforma de pagos y billetera virtual en América Latina.', true),
    ('OPENPAY', 'OpenPay', 'OpenPay BBVA (Tarjeta / Paynet)', 'PAYMENT', 'OpenPaySettings', 'MODULE_OPENPAY', 'MerchantId,PublicKey,PrivateKey', 'Pasarela de pagos respaldada por BBVA México con soporte para tarjetas y tiendas Paynet.', true),
    ('WHATSAPP', 'WhatsApp', 'WhatsApp Meta Cloud API', 'MESSAGING', 'WhatsAppSettings', 'MODULE_WHATSAPP', 'PhoneId,ApiToken', 'Envío y recepción de notificaciones a través de la API oficial de WhatsApp Cloud.', true),
    ('TICKETMASTER', 'Ticketmaster', 'Ticketmaster Discovery API v2', 'EVENTS', 'TicketmasterSettings', 'MODULE_EVENTS', 'ApiKey,ApiBaseUrl', 'Descubrimiento y sincronización automatizada de eventos, recintos (venues) y destinos.', true)
ON CONFLICT ("Code") DO UPDATE SET
    "Name" = EXCLUDED."Name",
    "DisplayName" = EXCLUDED."DisplayName",
    "Category" = EXCLUDED."Category",
    "SettingsTable" = EXCLUDED."SettingsTable",
    "LicenseModule" = EXCLUDED."LicenseModule",
    "RequiredColumns" = EXCLUDED."RequiredColumns",
    "Description" = EXCLUDED."Description";

-- 18.2 TicketmasterSettings
CREATE TABLE IF NOT EXISTS "TicketmasterSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Ticketmaster Discovery Settings',
    "ApiKey" TEXT,
    "ApiSecret" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://app.ticketmaster.com',
    "ApiVersion" VARCHAR(10) NOT NULL DEFAULT 'v2',
    "DefaultCountryCode" VARCHAR(5) NOT NULL DEFAULT 'MX',
    "DefaultCity" VARCHAR(100),
    "DefaultClassification" VARCHAR(100) DEFAULT 'Music',
    "AutoCreateRoutes" BOOLEAN NOT NULL DEFAULT false,
    "DefaultOriginPointCode" VARCHAR(50) DEFAULT 'DEP1',
    "SyncIntervalMinutes" INT NOT NULL DEFAULT 1440,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "LastSyncDate" TIMESTAMP,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- 19. StripeSettings
CREATE TABLE IF NOT EXISTS "StripeSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Stripe Payment Settings',
    "PublishableKey" TEXT,
    "SecretKey" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.stripe.com/v1',
    "CheckoutEndpoint" VARCHAR(255) NOT NULL DEFAULT '/checkout/sessions',
    "WebhookPath" VARCHAR(255) NOT NULL DEFAULT '/api/v1/stripe/webhook',
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- 24. StripeTransactions (Detailed Stripe Auditing)
CREATE TABLE IF NOT EXISTS "StripeTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "ReservationCode" VARCHAR(50) NOT NULL,
    "PaymentIntentId" VARCHAR(255) NOT NULL,
    "ChargeId" VARCHAR(255),
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'succeeded',
    "PaymentMethodType" VARCHAR(50) DEFAULT 'card',
    "Clabe" VARCHAR(50),
    "BankName" VARCHAR(100),
    "CardBrand" VARCHAR(50),
    "CardLast4" VARCHAR(10),
    "CardType" VARCHAR(50),
    "AuthorizationCode" VARCHAR(100),
    "CardFingerprint" VARCHAR(255),
    "ReceiptUrl" TEXT,
    "HostedInstructionsUrl" TEXT,
    "ClientIp" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_ReservationCode" ON "StripeTransactions" ("ReservationCode");
CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_PaymentIntentId" ON "StripeTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_Clabe" ON "StripeTransactions" ("Clabe");

-- 24.1 MercadoPagoSettings
CREATE TABLE IF NOT EXISTS "MercadoPagoSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Mercado Pago Settings',
    "PublicKey" TEXT,
    "AccessToken" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.mercadopago.com',
    "WebhookPath" VARCHAR(255) NOT NULL DEFAULT '/api/v1/mercadopago/webhook',
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

ALTER TABLE "MercadoPagoSettings" ADD COLUMN IF NOT EXISTS "WebhookPath" VARCHAR(255) NOT NULL DEFAULT '/api/v1/mercadopago/webhook';
ALTER TABLE "MercadoPagoSettings" ADD COLUMN IF NOT EXISTS "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn';

-- 24.2 MercadoPagoTransactions (Auditing & Webhook Correlation)
CREATE TABLE IF NOT EXISTS "MercadoPagoTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "ExternalReference" VARCHAR(50) NOT NULL,
    "PaymentId" VARCHAR(255) NOT NULL,
    "PreferenceId" VARCHAR(255),
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'pending',
    "StatusDetail" VARCHAR(255),
    "PaymentMethodId" VARCHAR(50),
    "PaymentTypeId" VARCHAR(50),
    "Installments" INT DEFAULT 1,
    "TicketUrl" TEXT,
    "Clabe" VARCHAR(50),
    "CardBrand" VARCHAR(50),
    "CardLast4" VARCHAR(10),
    "ClientIp" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_MercadoPagoTransactions_ExternalReference" ON "MercadoPagoTransactions" ("ExternalReference");
CREATE INDEX IF NOT EXISTS "IDX_MercadoPagoTransactions_PaymentId" ON "MercadoPagoTransactions" ("PaymentId");

-- 24.3 OpenPaySettings (Configuración Nativa de OpenPay)
CREATE TABLE IF NOT EXISTS "OpenPaySettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'OpenPay Settings',
    "MerchantId" VARCHAR(255),
    "PublicKey" TEXT,
    "PrivateKey" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://sandbox-api.openpay.mx/v1',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "OpenPaySettings" ("Code", "Name", "IsTestMode", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'OpenPay Settings', true, true, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 25. WhatsAppSettings (Meta Cloud API)
CREATE TABLE IF NOT EXISTS "WhatsAppSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE DEFAULT 'DEFAULT',
    "Name" VARCHAR(255) NOT NULL DEFAULT 'MetaConfig',
    "PhoneId" VARCHAR(255),
    "ApiToken" TEXT,
    "BusinessAccountId" VARCHAR(255),
    "WebhookVerifyToken" TEXT,
    "ApiVersion" VARCHAR(50) NOT NULL DEFAULT 'v24.0',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "WhatsAppSettings" ("Code", "Name", "PhoneId", "ApiToken", "BusinessAccountId", "WebhookVerifyToken", "ApiVersion", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'MetaConfig', '', '', '', '', 'v24.0', true, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 26. WhatsAppConversations
CREATE TABLE IF NOT EXISTS "WhatsAppConversations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "CustomerPhone" VARCHAR(50) NOT NULL,
    "CustomerName" VARCHAR(255),
    "LastMessageText" TEXT,
    "LastMessageDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "UnreadCount" INT NOT NULL DEFAULT 0,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'OPEN',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppConversations_CustomerPhone" ON "WhatsAppConversations" ("CustomerPhone");

-- 27. WhatsAppMessages
CREATE TABLE IF NOT EXISTS "WhatsAppMessages" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "WhatsAppId" VARCHAR(255),
    "ConversationEntry" INT NOT NULL,
    "SenderType" VARCHAR(50) NOT NULL DEFAULT 'OUTBOUND',
    "MessageType" VARCHAR(50) NOT NULL DEFAULT 'TEXT',
    "TemplateName" VARCHAR(255),
    "Content" TEXT,
    "MediaUrl" TEXT,
    "MediaId" VARCHAR(100),
    "MediaMimeType" VARCHAR(100),
    "MediaFileName" VARCHAR(255),
    "ContextWhatsAppId" VARCHAR(255),
    "MessageTimestamp" TIMESTAMP,
    "DeliveredDate" TIMESTAMP,
    "ReadDate" TIMESTAMP,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'SENT',
    "ResponsePayload" TEXT,
    "ErrorMessage" TEXT,
    "SentBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- Migraciones incrementales si la tabla ya existía previamente
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "MediaId" VARCHAR(100);
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "MediaMimeType" VARCHAR(100);
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "MediaFileName" VARCHAR(255);
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "ContextWhatsAppId" VARCHAR(255);
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "MessageTimestamp" TIMESTAMP;
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "DeliveredDate" TIMESTAMP;
ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "ReadDate" TIMESTAMP;

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_ConversationEntry" ON "WhatsAppMessages" ("ConversationEntry");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_Code" ON "WhatsAppMessages" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_WhatsAppId" ON "WhatsAppMessages" ("WhatsAppId");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_ContextWhatsAppId" ON "WhatsAppMessages" ("ContextWhatsAppId");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_MessageTimestamp" ON "WhatsAppMessages" ("MessageTimestamp");

-- =============================================================================
-- 28. WhatsApp Templates Modular Architecture (Meta Cloud API)
-- =============================================================================

-- Migration / Cleanup of legacy columns or tables
DO $$
BEGIN
    -- Drop obsolete tables if they exist
    DROP TABLE IF EXISTS "CustomAttachments" CASCADE;
    
    -- Alter WhatsAppTemplates to support MetaTemplateId, MetaStatus, RejectReason, Category
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = 'WhatsAppTemplates') THEN
        IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'MetaTemplateId') THEN
            ALTER TABLE "WhatsAppTemplates" ADD COLUMN "MetaTemplateId" VARCHAR(100);
        END IF;
        IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'RejectReason') THEN
            ALTER TABLE "WhatsAppTemplates" ADD COLUMN "RejectReason" TEXT;
        END IF;
        -- Drop legacy columns if they exist
        IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'ButtonsJson') THEN
            ALTER TABLE "WhatsAppTemplates" DROP COLUMN "ButtonsJson";
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'BodyTemplate') THEN
            ALTER TABLE "WhatsAppTemplates" DROP COLUMN "BodyTemplate";
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'HeaderContent') THEN
            ALTER TABLE "WhatsAppTemplates" DROP COLUMN "HeaderContent";
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'FooterText') THEN
            ALTER TABLE "WhatsAppTemplates" DROP COLUMN "FooterText";
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'WhatsAppTemplates' AND column_name = 'HeaderType') THEN
            ALTER TABLE "WhatsAppTemplates" DROP COLUMN "HeaderType";
        END IF;
    END IF;
END $$;


-- 28.1 WhatsAppTemplates (Master)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplates" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "TemplateName" VARCHAR(100) NOT NULL,
    "Language" VARCHAR(10) NOT NULL DEFAULT 'es_MX',
    "Category" VARCHAR(50) NOT NULL DEFAULT 'UTILITY',
    "MetaTemplateId" VARCHAR(100),
    "Status" VARCHAR(50) NOT NULL DEFAULT 'PENDING',
    "RejectReason" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_TemplateName" ON "WhatsAppTemplates" ("TemplateName");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_Code" ON "WhatsAppTemplates" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_MetaTemplateId" ON "WhatsAppTemplates" ("MetaTemplateId");

-- 28.2 WhatsAppTemplateHeaders (1:1 por plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateHeaders" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateEntry" INT NOT NULL REFERENCES "WhatsAppTemplates"("Entry") ON DELETE CASCADE,
    "HeaderType" VARCHAR(20) NOT NULL DEFAULT 'NONE', -- 'NONE', 'TEXT', 'IMAGE', 'DOCUMENT', 'VIDEO'
    "TextContent" TEXT,
    "MediaUrl" TEXT,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateHeader_TemplateEntry" UNIQUE ("TemplateEntry")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateHeaders_TemplateEntry" ON "WhatsAppTemplateHeaders" ("TemplateEntry");

-- 28.3 WhatsAppTemplateHeaderVariables (1:N variables del Header)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateHeaderVariables" (
    "Entry" SERIAL PRIMARY KEY,
    "HeaderEntry" INT NOT NULL REFERENCES "WhatsAppTemplateHeaders"("Entry") ON DELETE CASCADE,
    "VarName" VARCHAR(50) NOT NULL,
    "VarIndex" INT NOT NULL DEFAULT 1,
    "ExampleValue" VARCHAR(255),
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateHeaderVar" UNIQUE ("HeaderEntry", "VarIndex")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateHeaderVars_HeaderEntry" ON "WhatsAppTemplateHeaderVariables" ("HeaderEntry");

-- 28.4 WhatsAppTemplateBodies (1:1 por plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateBodies" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateEntry" INT NOT NULL REFERENCES "WhatsAppTemplates"("Entry") ON DELETE CASCADE,
    "BodyText" TEXT NOT NULL,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateBody_TemplateEntry" UNIQUE ("TemplateEntry")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateBodies_TemplateEntry" ON "WhatsAppTemplateBodies" ("TemplateEntry");

-- 28.5 WhatsAppTemplateBodyVariables (1:N variables del Body)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateBodyVariables" (
    "Entry" SERIAL PRIMARY KEY,
    "BodyEntry" INT NOT NULL REFERENCES "WhatsAppTemplateBodies"("Entry") ON DELETE CASCADE,
    "VarName" VARCHAR(50) NOT NULL,
    "VarIndex" INT NOT NULL DEFAULT 1,
    "ExampleValue" VARCHAR(255),
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateBodyVar" UNIQUE ("BodyEntry", "VarIndex")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateBodyVars_BodyEntry" ON "WhatsAppTemplateBodyVariables" ("BodyEntry");

-- 28.6 WhatsAppTemplateFooters (1:1 por plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateFooters" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateEntry" INT NOT NULL REFERENCES "WhatsAppTemplates"("Entry") ON DELETE CASCADE,
    "FooterText" TEXT,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateFooter_TemplateEntry" UNIQUE ("TemplateEntry")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateFooters_TemplateEntry" ON "WhatsAppTemplateFooters" ("TemplateEntry");

-- 28.7 WhatsAppTemplateFooterVariables (1:N variables del Footer si aplica)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateFooterVariables" (
    "Entry" SERIAL PRIMARY KEY,
    "FooterEntry" INT NOT NULL REFERENCES "WhatsAppTemplateFooters"("Entry") ON DELETE CASCADE,
    "VarName" VARCHAR(50) NOT NULL,
    "VarIndex" INT NOT NULL DEFAULT 1,
    "ExampleValue" VARCHAR(255),
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateFooterVar" UNIQUE ("FooterEntry", "VarIndex")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateFooterVars_FooterEntry" ON "WhatsAppTemplateFooterVariables" ("FooterEntry");

-- 28.8 WhatsAppTemplateButtons (1:N botones interactivos de la plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateButtons" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateEntry" INT NOT NULL REFERENCES "WhatsAppTemplates"("Entry") ON DELETE CASCADE,
    "ButtonType" VARCHAR(30) NOT NULL DEFAULT 'QUICK_REPLY', -- 'QUICK_REPLY', 'URL', 'PHONE_NUMBER'
    "Title" VARCHAR(100) NOT NULL,
    "Payload" TEXT, -- URL, Teléfono o ID
    "SortOrder" INT NOT NULL DEFAULT 1,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateButtons_TemplateEntry" ON "WhatsAppTemplateButtons" ("TemplateEntry");

-- Migration / Cleanup of legacy CustomMessages tables
DO $$
BEGIN
    DROP TABLE IF EXISTS "CustomAttachments" CASCADE;
    DROP TABLE IF EXISTS "CustomButtons" CASCADE;
    DROP TABLE IF EXISTS "CustomMessageParameters" CASCADE;
    DROP TABLE IF EXISTS "CustomMessages" CASCADE;
END $$;




-- =============================================================================
-- 34. SystemLicenses
--     Almacena la OmniLicense API Key activa para persistencia entre reinicios.
--     La VALIDACIÓN criptográfica se realiza en RAM (LicenseService / C++).
--     Solo una licencia puede estar activa (IsActive = true) a la vez.
-- =============================================================================
CREATE TABLE IF NOT EXISTS "SystemLicenses" (
    "Entry"       SERIAL PRIMARY KEY,
    "Code"        VARCHAR(50) NOT NULL UNIQUE,
    "ApiKey"      TEXT NOT NULL,
    "ClientName"  VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Licensee',
    "Issuer"      VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Authority',
    "IssuedAt"    DATE,
    "ExpiresAt"   DATE NOT NULL,
    "Modules"     TEXT NOT NULL DEFAULT '[]',
    "IsActive"    BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate"  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "UpdateDate"  TIMESTAMP
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_SystemLicenses_Active"
    ON "SystemLicenses" ("IsActive") WHERE "IsActive" = true;

-- Semilla de identidad para la generación de códigos LIC1, LIC2...
INSERT INTO "Identities" ("Domain", "Prefix1", "CurrentSequence")
VALUES ('SystemLicense', 'LIC', 0)
ON CONFLICT ("Domain") DO NOTHING;

-- =============================================================================
-- 35. Access Control & Security (Modules, Permissions, Roles, RolePermissions, UserPermissions, AuthorizationAuditLog)
-- =============================================================================

-- Identities seed for Roles, Permissions, Modules
INSERT INTO "Identities" ("Domain", "Prefix1", "CurrentSequence") VALUES
('Role', 'ROL', 0),
('Permission', 'PRM', 0),
('Module', 'MOD', 0)
ON CONFLICT ("Domain") DO NOTHING;

-- A. Modules
CREATE TABLE IF NOT EXISTS "Modules" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Description" TEXT,
    "Icon" VARCHAR(50) NOT NULL DEFAULT 'folder',
    "SortOrder" INT NOT NULL DEFAULT 1,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- B. Permissions
CREATE TABLE IF NOT EXISTS "Permissions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(150) NOT NULL,
    "Description" TEXT,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Permissions_ModuleCode" ON "Permissions" ("ModuleCode");

-- C. Roles
CREATE TABLE IF NOT EXISTS "Roles" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Description" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- D1. RoleModules
CREATE TABLE IF NOT EXISTS "RoleModules" (
    "Entry" SERIAL PRIMARY KEY,
    "RoleCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_RoleModules" UNIQUE ("RoleCode", "ModuleCode")
);
CREATE INDEX IF NOT EXISTS "IDX_RoleModules_RoleCode" ON "RoleModules" ("RoleCode");
CREATE INDEX IF NOT EXISTS "IDX_RoleModules_ModuleCode" ON "RoleModules" ("ModuleCode");

-- D2. RolePermissions
CREATE TABLE IF NOT EXISTS "RolePermissions" (
    "Entry" SERIAL PRIMARY KEY,
    "RoleCode" VARCHAR(50) NOT NULL,
    "PermissionCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_RolePermissions" UNIQUE ("RoleCode", "PermissionCode")
);
CREATE INDEX IF NOT EXISTS "IDX_RolePermissions_RoleCode" ON "RolePermissions" ("RoleCode");

-- E1. UserModules
CREATE TABLE IF NOT EXISTS "UserModules" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "GrantedByCode" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_UserModules" UNIQUE ("UserCode", "ModuleCode")
);
CREATE INDEX IF NOT EXISTS "IDX_UserModules_UserCode" ON "UserModules" ("UserCode");
CREATE INDEX IF NOT EXISTS "IDX_UserModules_ModuleCode" ON "UserModules" ("ModuleCode");

-- E2. UserPermissions
CREATE TABLE IF NOT EXISTS "UserPermissions" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50) NOT NULL,
    "PermissionCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "GrantedByCode" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_UserPermissions" UNIQUE ("UserCode", "PermissionCode")
);
CREATE INDEX IF NOT EXISTS "IDX_UserPermissions_UserCode" ON "UserPermissions" ("UserCode");

-- F. AuthorizationAuditLog
CREATE TABLE IF NOT EXISTS "AuthorizationAuditLog" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50),
    "GrantedByCode" VARCHAR(50),
    "Module" VARCHAR(50),
    "Permission" VARCHAR(100),
    "ResourceCode" VARCHAR(50),
    "Status" BOOLEAN NOT NULL,
    "Reason" TEXT,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_AuthAudit_UserCode" ON "AuthorizationAuditLog" ("UserCode");
CREATE INDEX IF NOT EXISTS "IDX_AuthAudit_CreateDate" ON "AuthorizationAuditLog" ("CreateDate");

-- Seed Modules (11 Modules)
INSERT INTO "Modules" ("Code", "Name", "Description", "Icon", "SortOrder", "IsActive", "CreatedBy") VALUES
('MOD_OVERVIEW', 'Panel Analítico', 'Métricas, KPIs y estadísticas globales', 'dashboard', 1, true, 'system'),
('MOD_RESERVATIONS', 'Reservaciones', 'Gestión de boletos, pasajeros y cobros', 'confirmation_number', 2, true, 'system'),
('MOD_SCHEDULES', 'Horarios y Salidas', 'Programación de corridas y viajes', 'schedule', 3, true, 'system'),
('MOD_EVENTS', 'Eventos', 'Conciertos, festivales y eventos', 'event', 4, true, 'system'),
('MOD_ROUTES', 'Rutas de Viaje', 'Trazado de rutas y tarifas base', 'alt_route', 5, true, 'system'),
('MOD_CATALOGS', 'Catálogos Base', 'Recintos, paradas y puntos de abordaje', 'folder', 6, true, 'system'),
('MOD_EMPLOYEES', 'Personal', 'Registro de empleados y operadores', 'badge', 7, true, 'system'),
('MOD_CUSTOMERS', 'Clientes', 'Padrón de pasajeros y viajeros frecuentes', 'groups', 8, true, 'system'),
('MOD_USERS', 'Usuarios y Accesos', 'Cuentas de usuario y seguridad', 'manage_accounts', 9, true, 'system'),
('MOD_ROLES', 'Roles y Perfiles', 'Perfiles y asignación de permisos', 'security', 10, true, 'system'),
('MOD_SETTINGS', 'Configuración General', 'Parámetros del sistema e integraciones', 'settings', 11, true, 'system')
ON CONFLICT ("Code") DO UPDATE SET
    "Name" = EXCLUDED."Name",
    "Description" = EXCLUDED."Description",
    "Icon" = EXCLUDED."Icon",
    "SortOrder" = EXCLUDED."SortOrder",
    "IsActive" = true;

-- Seed Permissions (34 Atomic Operational Permissions)
INSERT INTO "Permissions" ("Code", "Name", "Description", "ModuleCode", "IsActive", "CreatedBy") VALUES
-- 01. Overview
('ROUTE_OVERVIEW_EXPORT', 'Exportar Métricas', 'Permite exportar resúmenes ejecutivos en Excel o PDF', 'MOD_OVERVIEW', true, 'system'),

-- 02. Reservaciones
('ROUTE_RESERVATION_CREATE', 'Crear Reservación', 'Permite registrar nuevas reservaciones en el sistema', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_UPDATE', 'Editar Reservación', 'Permite modificar datos del pasajero o viaje', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_STATUS', 'Cambiar Estatus', 'Permite cambiar estado de reservación (Confirmar, etc.)', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_PAY', 'Procesar Pagos', 'Permite validar o registrar cobros de reservaciones', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_CANCEL', 'Cancelar Reservación', 'Permite anular reservaciones y liberar asientos', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_DELETE', 'Eliminar Registro', 'Permite borrado lógico de reservaciones', 'MOD_RESERVATIONS', true, 'system'),

-- 03. Horarios
('ROUTE_SCHEDULE_CREATE', 'Crear Horario', 'Permite programar nuevas corridas y viajes', 'MOD_SCHEDULES', true, 'system'),
('ROUTE_SCHEDULE_UPDATE', 'Editar Horario', 'Permite modificar horas, capacidad o tarifas de viaje', 'MOD_SCHEDULES', true, 'system'),
('ROUTE_SCHEDULE_DELETE', 'Eliminar Horario', 'Permite dar de baja una corrida programada', 'MOD_SCHEDULES', true, 'system'),

-- 04. Eventos
('ROUTE_EVENT_CREATE', 'Crear Evento', 'Permite dar de alta nuevos conciertos o festivales', 'MOD_EVENTS', true, 'system'),
('ROUTE_EVENT_UPDATE', 'Editar Evento', 'Permite actualizar datos, fecha o imagen del evento', 'MOD_EVENTS', true, 'system'),
('ROUTE_EVENT_DELETE', 'Eliminar Evento', 'Permite retirar eventos del sistema', 'MOD_EVENTS', true, 'system'),

-- 05. Rutas
('ROUTE_ROUTE_CREATE', 'Trazar Ruta', 'Permite crear itinerarios y rutas de viaje', 'MOD_ROUTES', true, 'system'),
('ROUTE_ROUTE_UPDATE', 'Editar Ruta', 'Permite modificar paradas o precios de la ruta', 'MOD_ROUTES', true, 'system'),
('ROUTE_ROUTE_DELETE', 'Eliminar Ruta', 'Permite anular rutas existentes', 'MOD_ROUTES', true, 'system'),

-- 06. Catálogos Base (Recintos y Paradas)
('ROUTE_CATALOG_CREATE', 'Crear Catálogo', 'Permite agregar recintos o puntos de abordaje', 'MOD_CATALOGS', true, 'system'),
('ROUTE_CATALOG_UPDATE', 'Editar Catálogo', 'Permite actualizar información de recintos o paradas', 'MOD_CATALOGS', true, 'system'),
('ROUTE_CATALOG_DELETE', 'Eliminar Catálogo', 'Permite dar de baja recintos o puntos de abordaje', 'MOD_CATALOGS', true, 'system'),

-- 07. Personal (Employees) - CORE
('CORE_EMPLOYEE_CREATE', 'Crear Empleado', 'Permite registrar personal y operadores', 'MOD_EMPLOYEES', true, 'system'),
('CORE_EMPLOYEE_UPDATE', 'Editar Empleado', 'Permite actualizar datos del colaborador', 'MOD_EMPLOYEES', true, 'system'),
('CORE_EMPLOYEE_DELETE', 'Eliminar Empleado', 'Permite dar de baja colaboradores', 'MOD_EMPLOYEES', true, 'system'),

-- 08. Clientes (Customers) - CORE
('CORE_CUSTOMER_CREATE', 'Registrar Cliente', 'Permite registrar nuevos pasajeros', 'MOD_CUSTOMERS', true, 'system'),
('CORE_CUSTOMER_UPDATE', 'Editar Cliente', 'Permite actualizar información del cliente', 'MOD_CUSTOMERS', true, 'system'),
('CORE_CUSTOMER_DELETE', 'Eliminar Cliente', 'Permite dar de baja registros de clientes', 'MOD_CUSTOMERS', true, 'system'),

-- 09. Usuarios - CORE
('CORE_USER_CREATE', 'Crear Usuario', 'Permite crear cuentas de acceso al sistema', 'MOD_USERS', true, 'system'),
('CORE_USER_UPDATE', 'Editar Usuario', 'Permite actualizar accesos, correo y credenciales', 'MOD_USERS', true, 'system'),
('CORE_USER_DELETE', 'Eliminar Usuario', 'Permite desactivar cuentas de acceso', 'MOD_USERS', true, 'system'),

-- 10. Roles y Permisos - CORE
('CORE_ROLE_CREATE', 'Crear Rol', 'Permite definir nuevos perfiles de usuario', 'MOD_ROLES', true, 'system'),
('CORE_ROLE_UPDATE', 'Editar Rol', 'Permite modificar asignación de permisos y módulos', 'MOD_ROLES', true, 'system'),
('CORE_ROLE_DELETE', 'Eliminar Rol', 'Permite dar de baja perfiles de rol', 'MOD_ROLES', true, 'system'),
('CORE_ROLE_MANAGE', 'Administrar Roles', 'Permite gestionar perfiles y asignaciones de rol', 'MOD_ROLES', true, 'system'),
('CORE_PERM_MANAGE', 'Administrar Permisos', 'Permite modificar y conceder permisos específicos', 'MOD_ROLES', true, 'system'),

-- 11. Configuración del Sistema - CORE
('CORE_CONFIG_UPDATE', 'Actualizar Configuración', 'Permite ajustar parámetros generales y políticas', 'MOD_SETTINGS', true, 'system'),
('CORE_PAYMENT_CREATE', 'Crear Método de Pago', 'Permite dar de alta nuevas formas de cobro', 'MOD_SETTINGS', true, 'system'),
('CORE_PAYMENT_UPDATE', 'Editar Configuración de Pago', 'Permite editar comisiones y cuentas bancarias', 'MOD_SETTINGS', true, 'system'),
('CORE_PAYMENT_DELETE', 'Eliminar Método de Pago', 'Permite desactivar opciones de cobro', 'MOD_SETTINGS', true, 'system'),
('CORE_INTEGRATION_STRIPE', 'Configurar Stripe', 'Permite configurar API keys y webhooks de cobros', 'MOD_SETTINGS', true, 'system'),
('CORE_INTEGRATION_WHATSAPP', 'Configurar WhatsApp', 'Permite editar tokens y plantillas de Meta Cloud', 'MOD_SETTINGS', true, 'system')
ON CONFLICT ("Code") DO UPDATE SET
    "Name" = EXCLUDED."Name",
    "Description" = EXCLUDED."Description",
    "ModuleCode" = EXCLUDED."ModuleCode",
    "IsActive" = true;

-- Access Control & Permissions catalog seeded. No default roles created.

-- Migration: Ensure all CreatedBy and LastUpdatedBy columns are VARCHAR(50) on existing databases
DO $$
DECLARE
    tbl text;
    tables text[] := ARRAY[
        'Users', 'Employees', 'Identities', 'Roles', 'Permissions', 'RolePermissions', 'UserPermissions',
        'RoleModules', 'UserModules', 'Departments', 'AuthorizationTemplates', 'AuthorizationStages', 'AuthorizationRules', 
        'AuthorizationRuleUsers', 'AuthorizationRequests', 'AuthorizationDecisions',
        'PaymentMethods', 'PaymentMethodDetails', 'PaymentTransactions', 'Integrations',
        'StripeSettings', 'EmailSettings', 'SystemConfigs',
        'WhatsAppSettings', 'TwilioSettings', 'CustomMessages', 'CustomButtons',
        'Branches', 'TaxRates', 'UnitOfMeasures', 'Currencies', 'ExchangeRates', 'Series'
    ];
BEGIN
    FOREACH tbl IN ARRAY tables
    LOOP
        IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = tbl) THEN
            IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public' AND table_name = tbl AND column_name = 'CreatedBy' AND data_type != 'character varying') THEN
                BEGIN
                    EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" DROP DEFAULT', tbl);
                EXCEPTION WHEN OTHERS THEN NULL;
                END;
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" TYPE VARCHAR(50) USING CASE WHEN "CreatedBy"::TEXT IN (''0'', ''1'', '''', ''SYSTEM'') THEN ''system'' WHEN "CreatedBy" IS NULL THEN ''system'' ELSE LOWER("CreatedBy"::TEXT) END', tbl);
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" SET DEFAULT ''system''', tbl);
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" SET NOT NULL', tbl);
            END IF;

            IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public' AND table_name = tbl AND column_name = 'LastUpdatedBy' AND data_type != 'character varying') THEN
                BEGIN
                    EXECUTE format('ALTER TABLE %I ALTER COLUMN "LastUpdatedBy" DROP DEFAULT', tbl);
                EXCEPTION WHEN OTHERS THEN NULL;
                END;
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "LastUpdatedBy" TYPE VARCHAR(50) USING CASE WHEN "LastUpdatedBy" IS NULL OR "LastUpdatedBy"::TEXT IN (''0'', '''') THEN NULL WHEN "LastUpdatedBy"::TEXT IN (''1'', ''SYSTEM'') THEN ''system'' ELSE LOWER("LastUpdatedBy"::TEXT) END', tbl);
            END IF;

            BEGIN
                EXECUTE format('UPDATE %I SET "CreatedBy" = ''system'' WHERE "CreatedBy" = ''SYSTEM''', tbl);
                EXECUTE format('UPDATE %I SET "LastUpdatedBy" = ''system'' WHERE "LastUpdatedBy" = ''SYSTEM''', tbl);
            EXCEPTION WHEN OTHERS THEN NULL;
            END;
        END IF;
    END LOOP;
END $$;

