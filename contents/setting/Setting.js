/*
    Setting.js - Setting model
 */

angular.module('app').factory('Setting', function (EspResource) {
    /* 
        Settings is really a singleton, but we use a group save routes on the server 
     */
    return EspResource.group("setting");
});
