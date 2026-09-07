const config_module = require("./config")
const nodemailer = require('nodemailer')

//---------------发邮件的模块----------------
/**
 * 创建发送邮件的代理
 */
let transport = nodemailer.createTransport({
    host: 'smtp.163.com',
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user, // 发送方邮箱地址
        pass: config_module.email_pass // 邮箱授权码或者密码
    }
});

/**
 * 异步发送邮件的函数（基于Promise封装）
 * @param {Object} mailOptions_ - 邮件配置对象，需包含发件人、收件人、主题、正文等信息
 * @returns {Promise} - 返回Promise对象，成功时返回邮件发送结果，失败时返回错误
 */
function SendMail(mailOptions_) {
    // 返回一个新的Promise对象，将回调式异步操作转换为Promise形式.支持 async/await 语法，避免回调地狱（Callback Hell）
    return new Promise(function(resolve, reject) {
        // 调用邮件传输对象的sendMail方法（假设transport已预先配置，如Nodemailer的传输器）
        transport.sendMail(mailOptions_, function(error, info) {//transport.SendMail相当于一个异步函数，调用该函数后发送的结果是通过回调函数通知的
            /* 异步回调处理 */
            if (error) {
                // 失败处理：打印错误日志，并通过reject传递错误
                console.log('邮件发送失败:', error);
                reject(error); // 将错误传递给Promise的.catch()
            } else {
                // 成功处理：打印成功日志，并通过resolve传递结果
                console.log('邮件已成功发送：', info.response);
                resolve(info.response); // 将结果传递给Promise的.then()
            }
        });
    });
}
module.exports.SendMail = SendMail