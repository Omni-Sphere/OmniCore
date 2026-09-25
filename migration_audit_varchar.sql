-- Migration: Alter CreatedBy and LastUpdatedBy from INT to VARCHAR(20)
-- Idempotent PostgreSQL script

DO $$
DECLARE
    tbl text;
    tables text[] := ARRAY[
        'Users', 'Employees', 'Identities', 'Roles', 'Permissions', 'RolePermissions', 'UserPermissions',
        'Departments', 'AuthorizationTemplates', 'AuthorizationStages', 'AuthorizationRules', 
        'AuthorizationRuleUsers', 'AuthorizationRequests', 'AuthorizationDecisions',
        'PaymentMethods', 'PaymentMethodDetails', 'PaymentGateways', 'PaymentTransactions',
        'StripeSettings', 'OpenPaySettings', 'MercadoPagoSettings', 'EmailSettings', 'SystemConfigs',
        'WhatsAppSettings', 'TwilioSettings', 'CustomMessages', 'CustomButtons',
        'Branches', 'TaxRates', 'UnitOfMeasures', 'Currencies', 'ExchangeRates', 'Series',
        'Venues', 'Events', 'DeparturePoints', 'Routes', 'RouteStops', 'Schedules',
        'SchedulePickupPoints', 'ScheduleDropoffPoints', 'Tickets', 'Reservations',
        'NotificationContacts', 'NotificationSettings', 'RouteSystemSettings'
    ];
BEGIN
    FOREACH tbl IN ARRAY tables
    LOOP
        IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = tbl) THEN
            -- Check and alter CreatedBy
            IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public' AND table_name = tbl AND column_name = 'CreatedBy') THEN
                BEGIN
                    EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" DROP DEFAULT', tbl);
                EXCEPTION WHEN OTHERS THEN NULL;
                END;
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" TYPE VARCHAR(20) USING CASE WHEN "CreatedBy"::TEXT IN (''0'', ''1'', '''') THEN ''SYSTEM'' WHEN "CreatedBy" IS NULL THEN ''SYSTEM'' ELSE "CreatedBy"::TEXT END', tbl);
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" SET DEFAULT ''SYSTEM''', tbl);
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" SET NOT NULL', tbl);
                RAISE NOTICE 'Updated %.CreatedBy to VARCHAR(20)', tbl;
            END IF;

            -- Check and alter LastUpdatedBy
            IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public' AND table_name = tbl AND column_name = 'LastUpdatedBy') THEN
                BEGIN
                    EXECUTE format('ALTER TABLE %I ALTER COLUMN "LastUpdatedBy" DROP DEFAULT', tbl);
                EXCEPTION WHEN OTHERS THEN NULL;
                END;
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "LastUpdatedBy" TYPE VARCHAR(20) USING CASE WHEN "LastUpdatedBy"::TEXT IN (''0'', ''1'', '''') THEN NULL WHEN "LastUpdatedBy" IS NULL THEN NULL ELSE "LastUpdatedBy"::TEXT END', tbl);
                RAISE NOTICE 'Updated %.LastUpdatedBy to VARCHAR(20)', tbl;
            END IF;
        END IF;
    END LOOP;
END $$;
